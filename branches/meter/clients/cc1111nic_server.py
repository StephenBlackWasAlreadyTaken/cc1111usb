import os
import sys
import cmd
import socket
import threading

from cc1111usb.const import *

DATA_START_IDX = 4      # without the app/cmd/len bytes, the data starts at byte 4

class CC1111NIC_Server(cmd.Cmd):
    def __init__(self, nicidx=0, ip='0.0.0.0', nicport=900, cfgport=899, go=True):
        cmd.Cmd.__init__(self)

        self.nic = USBDongle(nicidx)
        self._ip = ip
        self._nicport = nicport
        self._nicsock = None
        self._cfgport = cfgport
        self._cfgsock = None
        self._cfgthread = None

        self.startConfigThread()

        if go:
            self.start()

    def start(self):
        self._go = True
        while go:
            # serve the NIC port
            try:
                self._nicsock = socket.socket()
                s = self._nicssock
                s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s.bind((self._ip, self._nicport))
                s.listen(100)
                while True:
                    # implement pipe between the usb RF NIC and the TCP socket
                    try:
                        self._nicsock = s.accept()
                        rs, addr = self._nicsock

                        while True:
                            x,y,z = select.select([rs, self.nic], [], [], .1)
                            #FIXME: need to make the NIC object play nice :)
                            if rs in x:
                                data = rs.recv(MAX_PACKET_SIZE)
                                self.nic.send(APP_NIC, NIC_XMIT, data)
                            if self.nic in x:
                                data = self.nic.recv(APP_NIC)
                                rs.sendall(data[DATA_START_IDX:])

                    except:
                        sys.excepthook(*sys.exc_info())



    def startConfigThread(self):
        self._cfgthread = threading.Thread(target=self._cfgRun)
        self._cfgthread.daemon = True
        self._cfgthread.start()

    def _cfgRun(self):
        self._cfgssock = socket.socket()
        s = self._cfgsock
        s.bind((self._ip, self._cfgport))
        s.listen(100)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        while True:
            try:
                self._cfgsock = s.accept()
                rs,addr = self._cfgsock
                print "== received cfg connection from %s:%d ==" % (addr)

                self.stdin = rs
                self.stdout = rs

                self.cmdloop()
            except:
                sys.excepthook(*sys.exc_info())
    def do_stop(self):
        """ stop the nic
        """
        pass

    """ the following configuration change routiness should be implemented:
    * RFMODE - take the radio in and out of RX/IDLE/TX/SCAL  (SCAL returns to IDLE)
    * CALIBRATE - force the radio to recalibrate.  VCO characteristics will change with temperature and supply voltage changes
    * MODULATION - set the RF modulation scheme.  values include "2FSK", "GFSK", "MSK", "OOK", "ASK".  note: GFSK/OOK/ASK only up to 250kbaud, MSK only above 26kbaud and no manchester encoding.
    * BAUD - set the baud to one of the big bauds (or calculate the DRATE_M/_E. also, set BW settings from this
    * BW - allow the setting of bandwidth settings separately
    * DRATE - allow the setting of datarate settings separately
    * CHAN_SPC - set channel spacing
    * CHANNL - set the channel
    * FREQ - set the base frequency.  CHANNL and CHAN_SPC are used to calculate positive offset from this.

    * VLEN - configure the NIC for variable-length packets.  provide max packet size (FLEN to switch to Fixed)
    * FLEN - configure the NIC for fixed-length packets.  provide packet size (VLEN to switch to Variable)

    * SYNCWORD - set the SYNC word (SYNC1 and SYNC0)
    * SYNCMODE - set the SYNCMODE.  values include "NONE", "15/16", "16/16", "CS", "CS15/16", "CS16/16", "CS30/32"
    * PQT - set the Preamble Quality Threshold.  provide the number of bits (multiple of 4) for PQT.  values will be rounded down.  0-3 disables PQT checking.

    * ADDR - configure the NIC's ADDRESS
    * ADDR_CHK - filter based on the optional address byte.  values include "NOCHK", "FULL", "BCAST", indicating no filtering, full filtering, and filtering with broadcasts

    * DATAWHITEN - configure data whitening, include 9-bit PN9 xor sequence in command
    * MANCHESTER - configure Manchester encoding to enhance successful transmission.  cannot use with MSK modulation or the FEC/Interleaver.
    * FEC - enable/disable Forward Error Correction.  only works with FIXED LENGTH packets.
    * DEM_DCFILT - enable/disable digital DC blocking filter before demodulator.  typically not good to muck with.

    * MAGN_TARGET - configure Carrier Sense
    * MAX_LNA_GAIN - configure Carrier Sense Threshold
    * MAX_DVGA_GAIN - configure Carrier Sense Threshold (use 
    * CARRIER_SENSE_ABS_THR - configure Carrier Sense Absolute Threshold - values include "6", "10", "14" indicating the dB increase in RSSI
    * CARRIER_SENSE_REL_THR - configure Carrier Sense Relative Threshold - values include "DISABLE", and -7 thru +7 to indicate dB from MAGN_TARGET setting 

    * CCA_MODE - select the Clear Channel Assessment mode.  values include "DISABLE", "RSSI", "RECVING", "BOTH".

    * PA_POWER - select which PATABLE to use for power settings (0-7) (see CC1110/CC1111 manual SWRS033G section 13.15 and 13.16)

    * FS_AUTOCAL - select mode of auto-VCO-calibration.  values include "ON", "OFF", "MANUAL", indicating that calibration should be done when turning the synthesizer ON/OFF or manually



    allow the direct setting of these registers (be very cautious.  the firmware expects certain things like return-to-RX, and no RX-timeout, etc..)
    * TEST2/TEST1/TEST0
    * AGCCTRL2/1/0
    * PKTCTRL1/0
    * PKTLEN
    * PKTSTATUS
    * MDMCFG4/3/2/1/0
    * MCSM2/1/0
    * DEVIATN
    * BSCFG / FOCCFG
    * FSCTRL1/0
    * FREQ2/1/0
    * FREND1/0
    * PATABLE7/6/5/4/3/2/1/0

    the following introspection/telemetry routines should be implemented:
    * RSSI
    * LQI
    * PKTSTATUS
    
    full configuration items:
    * show_config - print a represented string of the radio configuration
    * dump_config - print a hex representation of the radio configuration registers
    * 902_19200_gfsk - canned configuration (can be used as a base)
    * upload_config - configure the radio using a python repr string provided to the command
    * download_config - pull current config bytes from radio and dump them in a python repr string


    also of interest:
    * peek
    * poke
    * ping
    * debugcodes
    * reset


