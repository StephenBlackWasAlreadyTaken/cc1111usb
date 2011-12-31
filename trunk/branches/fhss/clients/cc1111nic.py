#!/usr/bin/env ipython
import sys, usb, threading, time, struct
#from chipcondefs import *
from cc1111client import *

APP_NIC =                       0x42
NIC_RECV =                      0x1
NIC_XMIT =                      0x2

NIC_SET_ID =                    0x3

FHSS_SET_CHANNELS =             0x10
FHSS_NEXT_CHANNEL =             0x11
FHSS_CHANGE_CHANNEL =           0x12
FHSS_SET_MAC_THRESHOLD =        0x13

FHSS_SET_STATE =                0x20
FHSS_GET_STATE =                0x21
FHSS_START_SYNC =               0x22
FHSS_START_HOPPING =            0x23
FHSS_STOP_HOPPING =             0x24


FHSS_STATE_NONHOPPING =         0
FHSS_STATE_DISCOVERY =          1
FHSS_STATE_SYNCHING =           2
FHSS_LAST_NONHOPPING_STATE =    FHSS_STATE_SYNCHING

FHSS_STATE_SYNCHED =            3
FHSS_STATE_SYNC_MASTER =        4
FHSS_STATE_SYNCINGMASTER =      5


FHSS_STATES = {}
for key,val in globals().items():
    if key.startswith("FHSS_STATE_"):
        FHSS_STATES[key] = val
        FHSS_STATES[val] = key
                

T2SETTINGS = {}
T2SETTINGS_24MHz = {
    100: (4, 147, 3),
    150: (5, 110, 3),
    200: (5, 146, 3),
    250: (5, 183, 3),
    }
T2SETTINGS_26MHz = {
    100: (4, 158, 3),
    150: (5, 119, 3),
    200: (5, 158, 3),
    250: (5, 198, 3),
    }

class FHSSNIC(USBDongle):
    def __init__(self, idx=0, debug=False):
        USBDongle.__init__(self, idx, debug)

    def RFxmit(self, data):
        self.send(APP_NIC, NIC_XMIT, "%c%s" % (len(data)+1, data))

    def RFrecv(self, timeout=100):
        return self.recv(APP_NIC, timeout)

    def changeChannel(self, chan):
        return self.send(APP_NIC, FHSS_CHANGE_CHANNEL, "%c" % (chan))

    def setChannels(self, channels=[]):
        chans = ''.join(["%c" % chan for chan in channels])
        length = struct.pack("<H", len(chans))
        
        
        return self.send(APP_NIC, FHSS_SET_CHANNELS, length + chans)

    def nextChannel(self):
        return self.send(APP_NIC, FHSS_NEXT_CHANNEL, '' )

    def setupHopping(self, ms, mhz=24):
        # FIXME: auto-calibrate...
        tickspd, t2pr, tip = T2SETTINGS[mhz][ms]

        t2ctl = self.peek(T2CTL) & 0xfc
        t2ctl |= tip

        clkcon = (self.peek(CLKCON) & 0xc7)
        clkcon |= (tickspd<<3)

        self.poke(TICKSPD, "%c" % clkcon)
        self.poke(T2PR, "%c" % t2pr)
        self.poke(TIP, "%c" % tip)


    def startHopping(self):
        return self.send(APP_NIC, FHSS_START_HOPPING, '')

    def stopHopping(self):
        return self.send(APP_NIC, FHSS_STOP_HOPPING, '')

    def setMACthreshold(self, value):
        return self.send(APP_NIC, FHSS_SET_MAC_THRESHOLD, struct.pack("<I",value))

    def setFHSSstate(self, state):
        return self.send(APP_NIC, FHSS_SET_STATE, struct.pack("<I",state))
        
    def getFHSSstate(self):
        state = self.send(APP_NIC, FHSS_GET_STATE, '')
        print repr(state)
        state = ord(state[4])
        return FHSS_STATES[state], state
                                
    def mac_SyncCell(self, CellID=0x0000):
        return self.send(APP_NIC, FHSS_START_SYNC, struct.pack("<H",CellID))
                
        
        return 1


if __name__ == "__main__":
    idx = 0
    if len(sys.argv) > 1:
        idx = int(sys.argv.pop())
    d = FHSSNIC(idx=idx)

