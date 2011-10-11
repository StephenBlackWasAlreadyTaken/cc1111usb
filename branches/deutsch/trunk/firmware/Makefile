CC=sdcc
CFLAGS=-Iinclude
CFLAGSold=--no-pack-iram $(CF)
LFLAGS=--xram-loc 0xF000 

apps = 
libs = $(apps) cc1111usb.rel cc1111rf.rel cc1111_vcom.rel cc1111_vcomdesc.rel
srcs = $(libs,.rel=.c) 


all: chronosapplication.hex
donfw: donapplication.hex
immefw: immeapplication.hex

%.rel : %.c
	$(CC) $(CFLAGS) -c $<

chronosapplication.hex: application.c $(libs)
	sdcc $(CFLAGS) $(LFLAGS) application.c $(libs) -DCHRONOSDONGLE
	packihx <application.ihx >bins/application.hex

vcomapplication.hex: application.c $(libs)
	sdcc $(CFLAGS) $(LFLAGS) application.c $(libs) -DCHRONOSDONGLE -DVIRTUAL_COM
	packihx <application.ihx >bins/application.hex

donapplication.hex: application.c $(libs)
	sdcc $(CFLAGS) $(LFLAGS) application.c $(libs) -DDONSDONGLES
	packihx <application.ihx >bins/application.hex

immeapplication.hex: application.c $(libs)
	sdcc $(CFLAGS) $(LFLAGS) application.c $(libs) -DIMMEDONGLE
	packihx <application.ihx >bins/application.hex

installvcomfw: vcomapplication.hex
	goodfet.cc erase
	goodfet.cc flash bins/application.hex
	goodfet.cc info

installchronosfw: chronosapplication.hex
	goodfet.cc erase
	goodfet.cc flash bins/application.hex
	goodfet.cc info

installdonfw: donapplication.hex
	goodfet.cc erase
	goodfet.cc flash bins/application.hex
	goodfet.cc info

installimmefw: immeapplication.hex
	goodfet.cc erase
	goodfet.cc flash bins/application.hex
	goodfet.cc info

verify: chronosapplication.hex
	goodfet.cc verify bins/application.hex

clean:
	rm -f *.ihx *.rel *.hex *.sym *.asm *.lst *.lnk *.map *.mem *.rst
