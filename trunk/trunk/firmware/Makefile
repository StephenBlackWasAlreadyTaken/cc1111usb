CC=sdcc
CFLAGS=-Iinclude
CFLAGSold=--no-pack-iram $(CF)
LFLAGS=--xram-loc 0xF000 

apps = cc1111rf.rel global.rel
libsvcom = $(apps) cc1111_vcom.rel cc1111_vcomdesc.rel cc1111usb.rel cc1111usbdebug.rel
libsusb  = $(apps) cc1111usb.rel cc1111usbdebug.rel
libsimme  = $(apps) immeio.rel immekeys.rel immefont.rel   #  immedisplay.rel immeterm.rel
#srcs = $(libs,.rel=.c) 


all: chronosapplication.hex
donfw: donapplication.hex
immefw: immeapplication.hex
testrecv: testrecvapplication.hex
testxmit: testxmitapplication.hex

%.rel : %.c
	$(CC) $(CFLAGS) -c $<

chronosapplication.hex: application.c $(libsusb)
	sdcc $(CFLAGS) $(LFLAGS) application.c $(libsusb) -DCHRONOSDONGLE
	packihx <application.ihx >bins/application.hex

donapplication.hex: application.c $(libsusb)
	sdcc $(CFLAGS) $(LFLAGS) application.c $(libsusb) -DDONSDONGLES
	packihx <application.ihx >bins/application.hex

immedongleapplication.hex: application.c $(libsusb)
	sdcc $(CFLAGS) $(LFLAGS) application.c $(libsusb) -DIMMEDONGLE -DIMME
	packihx <application.ihx >bins/application.hex

testxmitusb.hex: appNetworkTest.c $(libsusb)
	sdcc $(CFLAGS) $(LFLAGS) appNetworkTest.c $(libsusb) -DCHRONOSDONGLE -DTRANSMIT_TEST 
	packihx <appNetworkTest.ihx >bins/testxmit.hex

testrecvusb.hex: appNetworkTest.c $(libsusb)
	sdcc $(CFLAGS) $(LFLAGS) appNetworkTest.c $(libsusb) -DCHRONOSDONGLE -DRECEIVE_TEST
	packihx <appNetworkTest.ihx >bins/testrecv.hex

testxmitapplication.hex: appNetworkTest.c $(libsvcom)
	sdcc $(CFLAGS) $(LFLAGS) appNetworkTest.c $(libsvcom) -DCHRONOSDONGLE -DVIRTUAL_COM -DTRANSMIT_TEST 
	packihx <appNetworkTest.ihx >bins/testxmit.hex

testrecvapplication.hex: appNetworkTest.c $(libsvcom)
	sdcc $(CFLAGS) $(LFLAGS) appNetworkTest.c $(libsvcom) -DCHRONOSDONGLE -DVIRTUAL_COM -DRECEIVE_TEST
	packihx <appNetworkTest.ihx >bins/testrecv.hex

NIC.hex: appNIC.c $(libsusb)
	sdcc $(CFLAGS) $(LFLAGS) appNIC.c $(libsusb) -DCHRONOSDONGLE
	packihx <appNIC.ihx >bins/NIC.hex

Sniff.hex: appSniff.c $(libsusb)
	sdcc $(CFLAGS) $(LFLAGS) appSniff.c $(libsusb) -DCHRONOSDONGLE
	packihx <appSniff.ihx >bins/Sniff.hex

immeSniff.hex: appSniff.c $(libsimme) 
	sdcc $(CFLAGS) $(LFLAGS) appSniff.c $(libsimme) -DIMME
	packihx <appSniff.ihx >bins/immeSniff.hex



installchronosfw: chronosapplication.hex
	goodfet.cc erase
	goodfet.cc flash bins/application.hex
	goodfet.cc info

installdonfw: donapplication.hex
	goodfet.cc erase
	goodfet.cc flash bins/application.hex
	goodfet.cc info

installimmefw: immedongleapplication.hex
	goodfet.cc erase
	goodfet.cc flash bins/application.hex
	goodfet.cc info

installtestrecvusbfw: testrecvusb.hex
	goodfet.cc erase
	goodfet.cc flash bins/testrecv.hex
	goodfet.cc info

installtestxmitusbfw: testxmitusb.hex
	goodfet.cc erase
	goodfet.cc flash bins/testxmit.hex
	goodfet.cc info

installtestrecvfw: testrecvapplication.hex
	goodfet.cc erase
	goodfet.cc flash bins/testrecv.hex
	goodfet.cc info

installtestxmitfw: testxmitapplication.hex
	goodfet.cc erase
	goodfet.cc flash bins/testxmit.hex
	goodfet.cc info

installnicfw: NIC.hex
	goodfet.cc erase
	goodfet.cc flash bins/NIC.hex
	goodfet.cc info

installsnifffw: Sniff.hex
	goodfet.cc erase
	goodfet.cc flash bins/Sniff.hex
	goodfet.cc info

installimmesnifffw: immeSniff.hex
	goodfet.cc erase
	goodfet.cc flash bins/immeSniff.hex
	goodfet.cc info


testall: chronosapplication.hex donapplication.hex immedongleapplication.hex testrecvusb.hex testxmitusb.hex testrecvapplication.hex testxmitapplication.hex NIC.hex Sniff.hex immeSniff.hex

verify: chronosapplication.hex
	goodfet.cc verify bins/application.hex

clean:
	rm -f *.ihx *.rel *.hex *.sym *.asm *.lst *.lnk *.map *.mem *.rst
