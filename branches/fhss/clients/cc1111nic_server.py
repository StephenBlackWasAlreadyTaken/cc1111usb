import os
import sys
import cmd
import socket
import threading

from cc1111client import *

DATA_START_IDX = 4      # without the app/cmd/len bytes, the data starts at byte 4

def splitargs(cmdline):
    cmdline = cmdline.replace('\\\\"', '"').replace('\\"', '')
    patt = re.compile('\".+?\"|\S+')
    for item in cmdline.split('\n'):
        return [s.strip('"') for s in patt.findall(item)]


RX = RFST_SRX
TX = RFST_STX
IDLE = RFST_SIDLE
CAL = RFST_SCAL

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
        while self._go:
            # serve the NIC port
            try:
                self._nicsock = socket.socket()
                s = self._nicsock
                s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s.bind((self._ip, self._nicport))
                s.listen(100)
                while True:
                    # implement pipe between the usb RF NIC and the TCP socket
                    try:
                        print >>sys.stderr,("Listening for NIC connection on port %d" % self._nicport)
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
            except:
                sys.excepthook(*sys.exc_info())



    def startConfigThread(self):
        self._cfgthread = threading.Thread(target=self._cfgRun)
        self._cfgthread.setDaemon(True)
        self._cfgthread.start()

    def _cfgRun(self):
        self._cfgsock = socket.socket()
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

    intro = """
        welcome to the cc1111usb interactive config tool.  hack fun!
        """

    def do_EOF(self, line):
        print >>self.stdout, "stopping the command loop now.."


    def do_stop(self, line):
        """ 
        stop the nic
        """
        pass

    #### configuration ####
    def do_rfmode(self, line):
        '''
        * RFMODE - set the radio in RX/IDLE/TX/CAL  (CAL returns to IDLE)
        '''
        if len(line):
            self.poke(X_RFST, eval(line))
        else:
            print >>self.stdout, self.getMARCSTATE()

    def do_calibrate(self, line):
        '''
        * CALIBRATE - force the radio to recalibrate.  VCO characteristics will change with temperature and supply voltage changes
        '''
        print >>self.stdout, "Calibrating radio..."
        self.setModeCAL()
        while (self.getMARCSTATE()[1] not in (MARC_STATE_IDLE, MARC_STATE_RX, MARC_STATE_TX)):
            sys.stdout.write('.')
        print >>self.stdout, "done calibrating."

    def do_modulation(self, line):
        '''
        * MODULATION - set the RF modulation scheme.  values include "2FSK", "GFSK", "MSK", "ASK_OOK".  note: GFSK/OOK/ASK only up to 250kbaud, MSK only above 26kbaud and no manchester encoding.
        '''
        if len(line) or line not in ("2FSK", "GFSK", "MSK", "ASK_OOK"):
            print >>self.stdout, 'need to give me one of the values "2FSK", "GFSK", "MSK", "ASK_OOK"'

        mod = eval("MOD_"+line.strip())
        self.setModeIDLE()
        self.setMdmModulation(mod)
        self.setModeRX()


    def complete_modulation(self, text, line, begidx, endidx):
        print >>self.stdout, "complete_modulation:  %s %s %s %s" % (repr(text), repr(line), repr(begidx), repr(endidx))

    def do_baud(self, line):
        '''
        * BAUD - set the baud to one of the big bauds (or calculate the DRATE_M/_E. also, set BW settings from this
        '''
        pass
        self.setModeIDLE()

        self.setModeRX()

    def do_bw(self, line):
        '''
        * BW <CHANBW_M> <CHANBW_E> - allow the setting of bandwidth settings separately from "baud"
        '''
        self.setModeIDLE()

        self.setModeRX()
        

    def do_drate(self, line):
        '''
        * DRATE - allow the setting of datarate settings separately from "baud"
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_chanspc(self, line):
        '''
        * CHANSPC - set channel spacing
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_channel(self, line):
        '''
        * CHANNEL - set the channel

        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_freq(self, line):
        '''
        * FREQ - set the base frequency.  CHANNL and CHAN_SPC are used to calculate positive offset from this.
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_vlen(self, line):
        '''
        * VLEN - configure the NIC for variable-length packets.  provide max packet size (FLEN to switch to Fixed)
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_flen(self, line):
        '''
        * FLEN # - configure the NIC for fixed-length packets.  provide packet size (VLEN to switch to Variable)
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_syncword(self, line):
        '''
        * SYNCWORD #### [double]- set the SYNC word (SYNC1 and SYNC0)  (double tells the radio to repeat SYNCWORD twice)
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_syncmode(self, line):
        '''
        * SYNCMODE - set the SYNCMODE.  values include "NONE", "15/16", "16/16", "CS", "CS15/16", "CS16/16", "CS30/32"
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_pqt(self, line):
        '''
        * PQT - set the Preamble Quality Threshold.  provide the number of bits (multiple of 4) for PQT.  values will be rounded down.  0-3 disables PQT checking.
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_addr(self, line):
        '''
        * ADDR - configure the NIC's ADDRESS
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_addr_chk(self, line):
        '''
        * ADDR_CHK - filter based on the optional address byte.  values include "NOCHK", "FULL", "BCAST", indicating no filtering, full filtering, and filtering with broadcasts
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_datawhiten(self, line):
        '''
        * DATAWHITEN - configure data whitening, include 9-bit PN9 xor sequence in command
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_manchester(self, line):
        '''
        * MANCHESTER - configure Manchester encoding to enhance successful transmission.  cannot use with MSK modulation or the FEC/Interleaver.
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_fec(self, line):
        '''
        * FEC - enable/disable Forward Error Correction.  only works with FIXED LENGTH packets.
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_DEM_DCFILT(self, line):
        '''
        * DEM_DCFILT - enable/disable digital DC blocking filter before demodulator.  typically not good to muck with.
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_MAGN_TARGET(self, line):
        '''
        * MAGN_TARGET - configure Carrier Sense
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_MAC_LNA_GAIN(self, line):
        '''
        * MAX_LNA_GAIN - configure Carrier Sense Threshold
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_MAX_DVGA_GAIN(self, line):
        '''
        * MAX_DVGA_GAIN - configure Carrier Sense Threshold (use 
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_CARRIER_SENSE_ABS_THR (self, line):
        '''
        * CARRIER_SENSE_ABS_THR - configure Carrier Sense Absolute Threshold - values include "6", "10", "14" indicating the dB increase in RSSI
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_CARRIER_SENSE_REL_THR (self, line):
        '''
        * CARRIER_SENSE_REL_THR - configure Carrier Sense Relative Threshold - values include "DISABLE", and -7 thru +7 to indicate dB from MAGN_TARGET setting 
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_cca_mode (self, line):
        '''
        * CCA_MODE - select the Clear Channel Assessment mode.  values include "DISABLE", "RSSI", "RECVING", "BOTH".
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_PA_POWER (self, line):
        '''
        * PA_POWER - select which PATABLE to use for power settings (0-7) (see CC1110/CC1111 manual SWRS033G section 13.15 and 13.16)
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_FS_AUTOCAL (self, line):
        '''
        * FS_AUTOCAL - select mode of auto-VCO-calibration.  values include "ON", "OFF", "MANUAL", indicating that calibration should be done when turning the synthesizer ON/OFF or manually
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_REGS_TEST(self, line):
        '''
        * set register:  TEST2/TEST1/TEST0
        (be very cautious!  the firmware expects certain things like return-to-RX, and no RX-timeout, etc..)
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_REGS_AGCCTRL(self, line):
        '''
        * set register:  AGCCTRL2/1/0
        (be very cautious!  the firmware expects certain things like return-to-RX, and no RX-timeout, etc..)
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_REG_PKTCTRL(self, line):
        '''
        * set register:  PKTCTRL1/0
        (be very cautious!  the firmware expects certain things like return-to-RX, and no RX-timeout, etc..)
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_REG_PKTLEN(self, line):
        '''
        * set register:  PKTLEN
        (be very cautious!  the firmware expects certain things like return-to-RX, and no RX-timeout, etc..)
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_REG_PKTSTATUS(self, line):
        '''
        * view register:  PKTSTATUS
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_REGS_MDMCFG(self, line):
        '''
        * set registers:  MDMCFG4/3/2/1/0
        (be very cautious!  the firmware expects certain things like return-to-RX, and no RX-timeout, etc..)
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_REGS_MCSM(self, line):
        '''
        * set register:  MCSM2/1/0
        (be very cautious!  the firmware expects certain things like return-to-RX, and no RX-timeout, etc..)
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_REG_DEVIATN(self, line):
        '''
        * set register:  DEVIATN
        (be very cautious!  the firmware expects certain things like return-to-RX, and no RX-timeout, etc..)
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_REGS_BSCFG_F0CCFG(self, line):
        '''
        * set register:  BSCFG / FOCCFG
        (be very cautious!  the firmware expects certain things like return-to-RX, and no RX-timeout, etc..)
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_REGS_FSCTRL(self, line):
        '''
        * set register:  FSCTRL1/0
        (be very cautious!  the firmware expects certain things like return-to-RX, and no RX-timeout, etc..)
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_REGS_FREQ(self, line):
        '''
        * set register:  FREQ2/1/0
        (be very cautious!  the firmware expects certain things like return-to-RX, and no RX-timeout, etc..)
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_REGS_FREND(self, line):
        '''
        * set register:  FREND1/0
        (be very cautious!  the firmware expects certain things like return-to-RX, and no RX-timeout, etc..)
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_REGS_PATABLE(self, line):
        '''
        * set register:  PATABLE7/6/5/4/3/2/1/0
        (be very cautious!  the firmware expects certain things like return-to-RX, and no RX-timeout, etc..)
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_show_config(self, line):
        '''
        * show_config - print >>self.stdout, a represented string of the radio configuration
        '''
        print >>self.stdout, self.reprRadioConfig()

    def do_dump_config(self, line):
        '''
        * dump_config - print >>self.stdout, a hex representation of the radio configuration registers
        '''
        print >>self.stdout, repr(self.getRadioConfig())

    def do_config_902_250k_gfsk(self, line):
        '''
        * 902_250k_gfsk - canned configuration (can be used as a base)
        '''
        self.setup_rfstudio_902PktTx()

    def do_hack_loose_settings(self, line):
        '''
        * loose - no CRC, no FEC, no Data Whitening, no sync-word, carrier based receive, etc...
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_upload_config(self, line):
        '''
        * upload_config - configure the radio using a python repr string provided to the command
        (be very cautious!  the firmware expects certain things like return-to-RX, and no RX-timeout, etc..)
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_download_config(self, line):
        '''
        * download_config - pull current config bytes from radio and dump them in a python repr string
        '''

    def do_save_config(self, line):
        '''
        * save_config <filename> - save the radio configuration to a file you specify
        '''

    def do_load_config(self, line):
        '''
        * load_config
        (be very cautious!  the firmware expects certain things like return-to-RX, and no RX-timeout, etc..)
        '''
        self.setModeIDLE()

        self.setModeRX()

    def do_peek(self, line):
        '''
        * peek <xaddr> [len] - view memory at whatever XDATA address (see the code for details on the memory layout)
        '''
        args = splitargs(line)
        if 0 < len(args) < 3:
            if len(args) == 1:
                args.append('1')
            print >>self.stdout, self.peek(int(args[0])), int(args[1]).encode('hex')
        else:
            print >>self.stdout, "please provide exactly one xdata address and an optional length!"

    def do_poke(self, line):
        '''
        * poke - update memory at whatever XDATA address (see the code for details on the memory layout)
        '''
        args = splitargs(line)
        try:
            self.poke(int(args[0]), args[1].decode('hex'))
        except:
            print >>self.stdout, "please provide exactly one xdata address and hex data"


    def do_ping(self, line):
        '''
        * ping - hello?  is the dongle still responding?
        '''
        print >>self.stdout, "Successful: %d, Failed: %d, Time: %f" % self.ping()

    def do_debug_codes(self, line):
        '''
        * debugcodes - see what the firmware has stored in it's lastCode[] array
        '''
        print >>self.stdout, "lastcode:  [%x, %x]" % (self.getDebugCodes())

    def do_RESET(self, line):
        '''
        * reset the dongle
        '''
        print >>self.stdout, "Sending the RESET command.  Please be patient...."
        self.RESET()

    def do_rssi(self, line):
        '''
        * get the last RSSI value
        '''
        print >>self.stdout, "RSSI: %x" % (ord(self.getRSSI()) & 0x7f)

    def do_(self, line):
        '''
        * get the last LQI value
        '''
        print >>self.stdout, "%x" % (ord(self.getLQI()))



if __name__ == "__main__":
    dongleserver = CC1111NIC_Server()
    
