import sys, usb, threading, time, struct

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

DEBUG_CMD_STRING                = 0xf0
DEBUG_CMD_HEX                   = 0xf1
DEBUG_CMD_HEX16                 = 0xf2
DEBUG_CMD_HEX32                 = 0xf3
DEBUG_CMD_INT                   = 0xf4



class USBDongle:
    ######## INITIALIZATION ########
    def __init__(self, debug=False):
        self.cleanup()
        self._debug = debug
        self._threadGo = False
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
        if self._debug: print >>sys.stderr,"XMIT:"+repr(buf)
        self._do.bulkWrite(5, "\x00\x00\x00\x00" + buf, timeout)

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
         return r

     def poke(self, addr, data):
         r = self.send(APP_SYSTEM, SYS_CMD_PEEK, struct.pack("<H", addr) + data)
         return r
 


    ######## APPLICATION METHODS ########


if __name__ == "__main__":
    d = USBDongle()

