import sys, usb, threading, time, struct
from chipcondefs import *

USB_BM_REQTYPE_TGTMASK          =0x1f
USB_BM_REQTYPE_TGT_DEV          =0x00
USB_BM_REQTYPE_TGT_INTF         =0x01
USB_BM_REQTYPE_TGT_EP           =0x02

USB_BM_REQTYPE_TYPEMASK         =0x60
USB_BM_REQTYPE_TYPE_STD         =0x00
USB_BM_REQTYPE_TYPE_CLASS       =0x20
USB_BM_REQTYPE_TYPE_VENDOR      =0x40
USB_BM_REQTYPE_TYPE_RESERVED    =0x60

USB_BM_REQTYPE_DIRMASK          =0x80
USB_BM_REQTYPE_DIR_OUT          =0x00
USB_BM_REQTYPE_DIR_IN           =0x80

USB_GET_STATUS                  =0x00
USB_CLEAR_FEATURE               =0x01
USB_SET_FEATURE                 =0x03
USB_SET_ADDRESS                 =0x05
USB_GET_DESCRIPTOR              =0x06
USB_SET_DESCRIPTOR              =0x07
USB_GET_CONFIGURATION           =0x08
USB_SET_CONFIGURATION           =0x09
USB_GET_INTERFACE               =0x0a
USB_SET_INTERFACE               =0x11
USB_SYNCH_FRAME                 =0x12

APP_GENERIC                     = 0x01
APP_DEBUG                       = 0xfe
APP_SYSTEM                      = 0xff


SYS_CMD_PEEK                    = 0x80
SYS_CMD_POKE                    = 0x81
SYS_CMD_PING                    = 0x82
SYS_CMD_STATUS                  = 0x83
SYS_CMD_POKE_REG                = 0x84

DEBUG_CMD_STRING                = 0xf0
DEBUG_CMD_HEX                   = 0xf1
DEBUG_CMD_HEX16                 = 0xf2
DEBUG_CMD_HEX32                 = 0xf3
DEBUG_CMD_INT                   = 0xf4

EP5OUT_MAX_PACKET_SIZE          = 64


MODES = {}
lcls = locals()
for lcl in lcls.keys():
    if lcl.startswith("MARC_STATE_"):
        MODES[lcl] = lcls[lcl]
        MODES[lcls[lcl]] = lcl




class USBDongle:
    ######## INITIALIZATION ########
    def __init__(self, debug=False):
        self.cleanup()
        self._debug = debug
        self._threadGo = False
        self.radiocfg = RadioConfig()
        self.resetup()
        self.recv_thread = threading.Thread(target=self.run)
        self.recv_thread.setDaemon(True)
        self.recv_thread.start()

    def cleanup(self):
        self._usberrorcnt = 0;
        self.recv_queue = ''
        self.recv_mbox  = {}
        self.xmit_queue = []
        self.trash = []
        self.sema = threading.Semaphore()
    
    def setup(self, console=True):
        for bus in usb.busses():
            for dev in bus.devices:
                if dev.idProduct == 0x4715:
                    if console: print >>sys.stderr,(dev)
                    d=dev
        self._d = d
        self._do = d.open()
        self._do.claimInterface(0)
        self._threadGo = True

    def resetup(self, console=True):
        self._do=None
        if console: print >>sys.stderr,("waiting")
        while (self._do==None):
            try:
                self.setup(console)
            except:
                if console: sys.stderr.write('.')



    ########  BASE FOUNDATIONAL "HIDDEN" CALLS ########
    def _sendEP0(self, buf=None, timeout=5000):
        if buf == None:
            buf = 'HELLO THERE'
        return self._do.controlMsg(USB_BM_REQTYPE_TGT_EP|USB_BM_REQTYPE_TYPE_VENDOR|USB_BM_REQTYPE_DIR_OUT,0, "\x00\x00\x00\x00\x00\x00\x00\x00"+buf, 0x200, 0, timeout), buf

    def _recvEP0(self, timeout=100):
        retary = ["%c"%x for x in self._do.controlMsg(USB_BM_REQTYPE_TGT_EP|USB_BM_REQTYPE_TYPE_VENDOR|USB_BM_REQTYPE_DIR_IN, 0, 64, 0x200, 0, timeout)]
        if len(retary):
            return ''.join(retary)
        return ""

    def _sendEP5(self, buf=None, timeout=1000):
        if (buf==None):
            buf = "\xff\x82\x07\x00ABCDEFG"
        while (len(buf)>0):
            if (buf > EP5OUT_MAX_PACKET_SIZE-4):
                drain = buf[:EP5OUT_MAX_PACKET_SIZE-4]
                buf = buf[EP5OUT_MAX_PACKET_SIZE-4:]
            else:
                drain = buf[:]
            if self._debug: print >>sys.stderr,"XMIT:"+repr(drain)
            self._do.bulkWrite(5, "\x00\x00\x00\x00" + drain, timeout)

    def _recvEP5(self, timeout=100):
        retary = ["%c"%x for x in self._do.bulkRead(0x85, 500, timeout)]
        #retary = self._do.bulkRead(5, 500, timeout)
        if self._debug: print >>sys.stderr,"RECV:"+repr(retary)
        if len(retary):
            return ''.join(retary)
            #return retary
        return ''

    ######## TRANSMIT/RECEIVE THREADING ########
    def run(self):
        msg = ''
        self.threadcounter = 0

        while True:
            if (not self._threadGo): continue

            self.threadcounter = (self.threadcounter + 1) & 0xffffffff

            #### transmit stuff.  if any exists in the xmit_queue
            msgsent = False
            msgrecv = False
            try:
                if len(self.xmit_queue):
                    msg = self.xmit_queue.pop(0)
                    self._sendEP5(msg)
                    msgsent = True
                else:
                    if self._debug>3: sys.stderr.write("NoMsgToSend ")
            #except IndexError:
                #if self._debug==3: sys.stderr.write("NoMsgToSend ")
                #pass
            except:
                sys.excepthook(*sys.exc_info())


            #### handle debug application
            try:
                q = self.recv_mbox.get(APP_DEBUG, None)
                if (q != None and len(q)):
                    buf = q.pop(0)
                    cmd = ord(buf[1])
                    if self._debug > 1: print >>sys.stderr,("buf length: %x\t\t cmd: %x\t\t(%s)"%(len(buf), cmd, repr(buf)))
                    if (cmd == DEBUG_CMD_STRING):
                        if (len(buf) < 4):
                            if (len(q)):
                                buf2 = q.pop(0)
                                buf += buf2
                            q.insert(0,buf)
                            if self._debug: sys.stderr.write('*')
                        else:
                            length, = struct.unpack("<H", buf[2:4])
                            if self._debug >1: print >>sys.stderr,("len=%d"%length)
                            if (len(buf) < 4+length):
                                if (len(q)):
                                    buf2 = q.pop(0)
                                    buf += buf2
                                q.insert(0,buf)
                                if self._debug: sys.stderr.write('&')
                            else:
                                printbuf = buf[4:4+length]
                                requeuebuf = buf[4+length:]
                                if len(requeuebuf):
                                    if self._debug>1:  print >>sys.stderr,(" - DEBUG..requeuing %s"%repr(requeuebuf))
                                    q.insert(0,requeuebuf)
                                print >>sys.stderr,("DEBUG: "+repr(printbuf))
                    elif (cmd == DEBUG_CMD_HEX):
                        #print >>sys.stderr, repr(buf[4:])
                        print >>sys.stderr, "DEBUG: %x"%(struct.unpack("B", buf[4:]))
                    elif (cmd == DEBUG_CMD_HEX16):
                        #print >>sys.stderr, repr(buf[4:])
                        print >>sys.stderr, "DEBUG: %x"%(struct.unpack("<H", buf[4:]))
                    elif (cmd == DEBUG_CMD_HEX32):
                        #print >>sys.stderr, repr(buf[4:])
                        print >>sys.stderr, "DEBUG: %x"%(struct.unpack("<L", buf[4:]))
                    elif (cmd == DEBUG_CMD_INT):
                        print >>sys.stderr, "DEBUG: %d"%(struct.unpack("<L", buf[4:]))
                    else:
                        print >>sys.stderr,('DEBUG COMMAND UNKNOWN: %x (buf=%s)'%(cmd,repr(buf)))

            except:
                sys.excepthook(*sys.exc_info())

            #### receive stuff.
            try:
                #### first we populate the queue
                msg = self._recvEP5(40)
                if len(msg) > 0:
                    self.recv_queue += msg
                    msgrecv = True
                #while (len(self.recv_queue) and self.recv_queue[0] != 0x40):
                #    self.recv_queue = [1:]                     # true up to the next packet
                #self.recv_queue.pop(0)                          # get rid of the @ symbol


                #### now we parse, sort, and deliver the mail.
                idx = self.recv_queue.find('@')
                if (idx==-1):
                    sys.stderr.write('@')
                else:
                    if (idx>0):
                        self.trash.append(self.recv_queue[:idx])
                        self.recv_queue = self.recv_queue[idx:]
               
                    msg = self.recv_queue[1:]                           # pop off the leading "@"   really?  here?  before we know we're done?
                    msglen = len(msg)
                    if (msglen>=4):                                      # if not enough to parse length... we'll wait.
                        app = ord(msg[0])
                        cmd = ord(msg[1])
                        length, = struct.unpack("<H", msg[2:4])
                        if self._debug>1: print>>sys.stderr,("app=%x  cmd=%x  len=%x"%(app,cmd,length))
                        if (msglen >= length+4):
                            #### if the queue has enough characters to handle the next message... chop it and put it in the appropriate recv_mbox
                            msg = self.recv_queue[1:length+5]                   # drop the initial '@' and chop out the right number of chars
                            self.recv_queue = self.recv_queue[length+5:]        # chop it out of the queue

                            q = self.recv_mbox.get(app,None)
                            if (q == None):
                                q = []
                            self.sema.acquire()                            # THREAD SAFETY DANCE
                            q.append(msg)
                            self.recv_mbox[app] = q
                            self.sema.release()                            # THREAD SAFETY DANCE COMPLETE
                        else:            
                            if self._debug:     sys.stderr.write('=')
                    else:
                        if self._debug:     sys.stderr.write('.')
            except usb.USBError, e:
                #sys.stderr.write(repr(self.recv_queue))
                #sys.stderr.write(repr(e))
                self.sema.release()                            # THREAD SAFETY DANCE COMPLETE
                if self._debug>4: print >>sys.stderr,repr(sys.exc_info())
                if ('No such device' in repr(e)):
                    self._threadGo = False
                    self.resetup(False)
                self._usberrorcnt += 1
                pass
            except:
                self.sema.release()                            # THREAD SAFETY DANCE COMPLETE
                sys.excepthook(*sys.exc_info())


            if not (msgsent or msgrecv or len(msg)) :
                time.sleep(.1)
            else:
                if self._debug > 5:  sys.stderr.write(" %s:%s:%d .-P."%(msgsent,msgrecv,len(msg)))


                




    ######## APPLICATION API ########
    def recv(self, app, wait=100):
        for x in xrange(wait):
            try:
                q = self.recv_mbox.get(app, None)
                #print >>sys.stderr,"debug(recv) q='%s'"%repr(q)
                self.sema.acquire(False)
                resp = q.pop(0)
                self.sema.release()
                return resp
            except IndexError:
                #sys.excepthook(*sys.exc_info())
                self.sema.release()
                pass
            except AttributeError:
                #sys.excepthook(*sys.exc_info())
                self.sema.release()
                pass
            except:
                self.sema.release()
                sys.excepthook(*sys.exc_info())
            time.sleep(.2)                                      # only hits here if we don't have something in queue
    def recvAll(self, app):
        retval = self.recv_mbox.get(app,None)
        self.recv_mbox[app]=[]
        return retval

    def send(self, app, cmd, buf):
        #self._sendEP5("%c%c%s"%(app,cmd,buf))
        self.xmit_queue.append("%c%c%s%s"%(app,cmd, struct.pack("<H",len(buf)),buf))
        return self.recv(app)

    def getDebugCodes(self):
        x = self._recvEP0(1000)
        if (x != None):
            return struct.unpack("BB", x)

    def debug(self):
        while True:
            try:
                print >>sys.stderr, ("DONGLE RESPONDING:  mode :%x, last error# %d"%(self.getDebugCodes()))
            except:
                pass
            print >>sys.stderr,('recv_queue:\t\t (%d bytes) "%s"'%(len(self.recv_queue),repr(self.recv_queue)[:len(self.recv_queue)%39+20]))
            print >>sys.stderr,('trash:     \t\t (%d bytes) "%s"'%(len(self.trash),repr(self.trash)[:len(self.trash)%39+20]))
            print >>sys.stderr,('recv_mbox  \t\t (%d keys)  "%s"'%(len(self.recv_mbox),repr(self.recv_mbox)[:len(repr(self.recv_mbox))%79]))
            for x in self.recv_mbox.keys():
                print >>sys.stderr,('    recv_mbox   %d\t (%d records)  "%s"'%(x,len(self.recv_mbox[x]),repr(self.recv_mbox[x])[:len(repr(self.recv_mbox[x]))%79]))
            time.sleep(1)

    def ping(self, count=10, buf="ABCDEFGHIJKLMNOPQRSTUVWXYZ"):
        good=0
        bad=0
        for x in range(count):
            r = self.send(APP_SYSTEM, SYS_CMD_PING, buf)
            print "PING: %d bytes transmitted, received: %s"%(len(buf), repr(r))
            if r==None:
                bad+=1
            else:
                good+=1
        return (good,bad)

    def peek(self, addr, bytecount=1):
        r = self.send(APP_SYSTEM, SYS_CMD_PEEK, struct.pack("<HH", bytecount, addr))
        return r[4:]

    def poke(self, addr, data):
        r = self.send(APP_SYSTEM, SYS_CMD_POKE, struct.pack("<H", addr) + data)
        return r[4:]
    
    def pokeReg(self, addr, data):
        r = self.send(APP_SYSTEM, SYS_CMD_POKE_REG, struct.pack("<H", addr) + data)
        return r[4:]
    
    ######## RADIO METHODS #########
    def getRadioConfig(self):
        bytedef = self.peek(0xdf00, 0x3e)
        self.radiocfg.vsParse(bytedef)
        return bytedef

    def setRadioConfig(self):
        bytedef = self.radiocfg.vsEmit()
        self.poke(0xdf00, bytedef)
        return bytedef

    def setFreq(self, freq=902000000, mhz=24):
        freqmult = (0x10000 / 1000000.0) / mhz
        num = int(freq * freqmult)
        self.radiocfg.freq2 = num >> 16
        self.radiocfg.freq1 = (num>>8) & 0xff
        self.radiocfg.freq0 = num & 0xff
        self.poke(FREQ2, struct.pack("3B", self.radiocfg.freq2, self.radiocfg.freq1, self.radiocfg.freq0))

    def getFreq(self, mhz=24):
        freqmult = (0x10000 / 1000000.0) / mhz
        bytedef = self.peek(FREQ2, 3)
        if (len(bytedef) != 3):
            raise(Exception("unknown data returned for getFreq(): %s"%repr(bytedef)))
        (       self.radiocfg.freq2, 
                self.radiocfg.freq1, 
                self.radiocfg.freq0) = struct.unpack("3B", bytedef)
        num = (self.radiocfg.freq2<<16) + (self.radiocfg.freq1<<8) + self.radiocfg.freq0
        print >>sys.stderr,( hex(num))
        freq = num / freqmult
        return freq

    def getMARCSTATE(self):
        mode = ord(self.peek(MARCSTATE))
        return (MODES[mode], mode)

    def setModeTX(self):
        self.poke(X_RFST, "%c"%RFST_STX)

    def setModeRX(self):
        self.poke(X_RFST, "%c"%RFST_SRX)

    def setModeIDLE(self):
        self.poke(X_RFST, "%c"%RFST_SIDLE)

    def setModeTXRXON(self):
        self.poke(X_RFST, "%c"%RFST_SFSTXON)

    def setModeSCAL(self):
        self.poke(X_RFST, "%c"%RFST_SCAL)

    def getMdmModulation(self):
        mdmcfg2 = self.peek(MDMCFG2)
        mod = (mdmcfg2 >> 4) & 0x7
        mchstr = (mdmcfg2 >> 3) & 1
        return (mod,mchstr,MODULCATIONS[(mod<<1) | mchstr])

    def setMdmModulation(self, mod, mchstr=0):
        if (mod|mchstr) & 0x87:
            raise(Exception("Please use constants MOD_FORMAT_* to specify modulation and "))
        mdmcfg2 = ord(self.peek(MDMCFG2))
        mdmcfg2 |= (mod) | (mchstr)
        self.poke(MDMCFG2, struct.pack("<I",mdmcfg2)[0])

    def getMdmChanSpc(self, mhz=24):
        chanspc_m = ord(self.peek(MDMCFG0))
        chanspc_e = ord(self.peek(MDMCFG1)) & 3
        spacing = (mhz/0.262144) * (2**chanspc_e) * (256+chanspc_m)
        return (chanspc_m, chanspc_e, spacing)

    def setMdmChanSpc(self, chanspc_m, chanspc_e, spacing=None, mhz=24):
        if (spacing != None):
            tmp = spacing * 0x262144/mhz
            raise(Exception("setting channel spacing only supported using chanspc mantissa and exponent for now"))
        self.poke(MDMCFG0, "%c"%chanspc_m)
        mdmcfg1 = ord(self.peek(MDMCFG1)) & 0xfc  # clear out old exponent value
        mdmcfg1 |= chanspc_e
        self.poke(MDMCFG1, "%c"%mdmcfg1)

            
        

    ######## APPLICATION METHODS ########
    def setup900MHz(self):
        self.getRadioConfig()
        rc = self.radiocfg
        rc.pktctrl1   = 0xe5
        rc.pktctrl0   = 0x04
        rc.fsctrl1    = 0x12
        rc.fsctrl0    = 0x00
        rc.mdmcfg4    = 0x3e
        rc.mcsm0      = 0x00
        rc.agcctrl2  |= AGCCTRL2_MAX_DVGA_GAIN
        rc.fscal3     = 0xEA
        rc.fscal2     = 0x2A
        rc.fscal1     = 0x00
        rc.fscal0     = 0x1F
        rc.test2      = 0x88
        rc.test1      = 0x31
        rc.test0      = 0x09
        self.setRadioConfig()

    def setup900MHzHopTrans(self):
        self.getRadioConfig()
        rc = self.radiocfg
        rc.iocfg0     = 0x06
        rc.sync1      = 0x0b
        rc.sync0      = 0x0b
        rc.pktlen     = 0xff
        rc.pktctrl1   = 0x04
        rc.pktctrl0   = 0x05
        rc.addr       = 0x00
        rc.channr     = 0x00
        rc.fsctrl1    = 0x06
        rc.fsctrl0    = 0x00
        rc.mdmcfg4    = 0xee
        rc.mdmcfg3    = 0x55
        rc.mdmcfg2    = 0x73
        rc.mdmcfg1    = 0x23
        rc.mdmcfg0    = 0x55
        rc.mcsm2      = 0x07
        rc.mcsm1      = 0x30
        rc.mcsm0      = 0x18
        rc.deviatn    = 0x16
        rc.foccfg     = 0x17
        rc.bscfg      = 0x6c
        rc.agcctrl2   = 0x03
        rc.agcctrl1   = 0x40
        rc.agcctrl0   = 0x91
        rc.frend1     = 0x56
        rc.frend0     = 0x10
        rc.fscal3     = 0xEA
        rc.fscal2     = 0x2A
        rc.fscal1     = 0x00
        rc.fscal0     = 0x1F
        rc.test2      = 0x88
        rc.test1      = 0x31
        rc.test0      = 0x09
        self.setRadioConfig()

    def setup900MHzContTrans(self):
        self.getRadioConfig()
        rc = self.radiocfg
        rc.iocfg0     = 0x06
        rc.sync1      = 0x0b
        rc.sync0      = 0x0b
        rc.pktlen     = 0xff
        rc.pktctrl1   = 0x04
        rc.pktctrl0   = 0x05
        rc.addr       = 0x00
        rc.channr     = 0x00
        rc.fsctrl1    = 0x06
        rc.fsctrl0    = 0x00
        rc.freq2      = 0x26
        rc.freq1      = 0x55
        rc.freq0      = 0x55
        rc.mdmcfg4    = 0xee
        rc.mdmcfg3    = 0x55
        rc.mdmcfg2    = 0x73
        rc.mdmcfg1    = 0x23
        rc.mdmcfg0    = 0x55
        rc.mcsm2      = 0x07
        rc.mcsm1      = 0x30
        rc.mcsm0      = 0x18
        rc.deviatn    = 0x16
        rc.foccfg     = 0x17
        rc.bscfg      = 0x6c
        rc.agcctrl2   = 0x03
        rc.agcctrl1   = 0x40
        rc.agcctrl0   = 0x91
        rc.frend1     = 0x56
        rc.frend0     = 0x10
        rc.fscal3     = 0xEA
        rc.fscal2     = 0x2A
        rc.fscal1     = 0x00
        rc.fscal0     = 0x1F
        rc.test2      = 0x88
        rc.test1      = 0x31
        rc.test0      = 0x09
        rc.pa_table0  = 0xc0
        self.setRadioConfig()





if __name__ == "__main__":
    d = USBDongle()

