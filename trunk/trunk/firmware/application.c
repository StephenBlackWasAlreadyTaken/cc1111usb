#include "cc1111rf.h"
#include "global.h"

#ifdef VIRTUAL_COM
	#include "cc1111.h"
	#include "cc1111_vcom.h"
#else
	#include "cc1111usb.h"
#endif

/*************************************************************************************************
 * welcome to the cc1111usb application.
 * this lib was designed to be the basis for your usb-app on the cc1111 radio.  hack fun!
 *
 * 
 * best way to start is to look over the library and get a little familiar with it.
 * next, put code as follows:
 * * any initialization code that should happen at power up goes in appMainInit()
 * * the main application loop code should go in appMainLoop()
 * * usb interface code should go into appHandleEP5.  this includes a switch statement for any 
 *      verbs you want to create between the client on this firmware.
 *
 * if you should need to change anything about the USB descriptors, do your homework!  particularly
 * keep in mind, if you change the IN or OUT max packetsize, you *must* change it in the 
 * EPx_MAX_PACKET_SIZE define, the desciptor definition (be sure to get the right one!) and should 
 * correspond to the setting of MAXI and MAXO.
 * 
 * */




/*************************************************************************************************
 * Application Code - these first few functions are what should get overwritten for your app     *
 ************************************************************************************************/

void appMainInit(void)
{
    /* this indicates that we've enabled stuff to a good point */
    blink(100,10);
#ifdef RECEIVE_TEST
    startRX();
#endif
}

/* appMain is the application.  it is called every loop through main, as does the USB handler code.
 * please do not block if you want USB to work.                                                 */
void appMainLoop(void)
{
	xdata u8 processbuffer;

    if (rfif)
    {
        lastCode[0] = 0xd;
        IEN2 &= ~IEN2_RFIE;

        if(rfif & RFIF_IRQ_DONE)
        {
        	processbuffer = !rfRxCurrentBuffer;
			if(rfRxProcessed[processbuffer] == RX_UNPROCESSED)
			{
#ifdef VIRTUAL_COM
				vcom_putstr(rfrxbuf[processbuffer]);
				vcom_flush();
#else
				debug((code u8*)rfrxbuf[processbuffer]);
#endif
				/* Set receive buffer to processed so it can be used again */
				rfRxProcessed[processbuffer] = RX_PROCESSED;
			}
        }

        rfif = 0;
        IEN2 |= IEN2_RFIE;
    }
#ifdef TRANSMIT_TEST
	xdata u8 testPacket[13];

	 /* Send a packet */
	testPacket[0] = 0x0B;
	testPacket[1] = 0x48;
	testPacket[2] = 0x41;
	testPacket[3] = 0x4C;
	testPacket[4] = 0x4C;
	testPacket[5] = 0x4F;
	testPacket[6] = 0x43;
	testPacket[7] = 0x43;
	testPacket[8] = 0x31;
	testPacket[9] = 0x31;
	testPacket[10] = 0x31;
	testPacket[11] = 0x31;
	testPacket[12] = 0x00;

    transmit(testPacket, 13);
    blink(200,200);
#endif
}


/* appHandleEP5 gets called when a message is received on endpoint 5 from the host.  this is the 
 * main handler routine for the application as endpoint 0 is normally used for system stuff.
 *
 * important things to know:
 *  * your data is in ep5iobuf.OUTbuf, the length is ep5iobuf.OUTlen, and the first two bytes are
 *      going to be \x40\xe0.  just craft your application to ignore those bytes, as i have ni
 *      puta idea what they do.  
 *  * transmit data back to the client-side app through txdatai().  this function immediately 
 *      xmits as soon as any previously transmitted data is out of the buffer (ie. it blocks 
 *      while (ep5iobuf.flags & EP_INBUF_WRITTEN) and then transmits.  this flag is then set, and 
 *      cleared by an interrupt when the data has been received on the host side.                */
int appHandleEP5()
{   // not used by VCOM
#ifndef VIRTUAL_COM
    u8 app, cmd;
    u16 len;
    xdata u8 *buf;

    app = ep5iobuf.OUTbuf[4];
    cmd = ep5iobuf.OUTbuf[5];
    buf = &ep5iobuf.OUTbuf[6];
    len = (u16)*buf;
    buf += 2;                                               // point at the address in memory
    // ep5iobuf.OUTbuf should have the following bytes to start:  <app> <cmd> <lenlow> <lenhigh>
    // check the application
    //  then check the cmd
    //   then process the data
    switch (cmd)
    {
        default:
            break;
    }
    ep5iobuf.flags &= ~EP_OUTBUF_WRITTEN;                       // this allows the OUTbuf to be rewritten... it's saved until now.
#endif
    return 0;
}

/* in case your application cares when an OUT packet has been completely received.               */
void appHandleEP0OUTdone(void)
{
#ifndef VIRTUAL_COM
//code here
#endif
}

/* this function is the application handler for endpoint 0.  it is called for all VENDOR type    *
 * messages.  currently it implements a simple ping-like application.                           */
int appHandleEP0(USB_Setup_Header* pReq)
{
#ifdef VIRTUAL_COM
	pReq = 0;
#else
    if (pReq->bmRequestType & USB_BM_REQTYPE_DIRMASK)       // IN to host
    {
        switch (pReq->bRequest)
        {
            case 0:
                setup_send_ep0(&lastCode[0], 2);
                break;
            case 1:
                setup_sendx_ep0((xdata u8*)USBADDR, 40);
                break;
            case 2:
                setup_sendx_ep0((xdata u8*)pReq->wValue, pReq->wLength);
                break;

        }
    } else                                                  // OUT from host
    {
        if (pReq->wIndex&0xf)                               // EP0 receive.    CURRENTLY DOES NOTHING WITH THIS....
        {
            usb_recv_ep0OUT();
            ep0iobuf.flags &= ~EP_OUTBUF_WRITTEN;
        }
    }
#endif
    return 0;
}



/*************************************************************************************************
 *  here begins the initialization stuff... this shouldn't change much between firmwares or      *
 *  devices.                                                                                     *
 *************************************************************************************************/

/* initialize the IO subsystems for the appropriate dongles */
static void io_init(void)
{
#ifdef IMMEDONGLE   // CC1110 on IMME pink dongle
    // IM-ME Dongle.  It's a CC1110, so no USB stuffs.  Still, a bit of stuff to init for talking 
    // to it's own Cypress USB chip
    P0SEL |= (BIT5 | BIT3);     // Select SCK and MOSI as SPI
    P0DIR |= BIT4 | BIT6;       // SSEL and LED as output
    P0 &= ~(BIT4 | BIT2);       // Drive SSEL and MISO low

    P1IF = 0;                   // clear P1 interrupt flag
    IEN2 |= IEN2_P1IE;          // enable P1 interrupt
    P1IEN |= BIT1;              // enable interrupt for P1.1

    P1DIR |= BIT0;              // P1.0 as output, attention line to cypress
    P1 &= ~BIT0;                // not ready to receive
    
#else       // CC1111
    // this may need to be changed for DONSDONGLES... have DON check
    P0DIR |= 0x0F;
    P0_0 = 0;
    P0_1 = 0;
    P0_2 = 0;
    P0_3 = 0;

#ifdef DONSDONGLES
    // CC1111 USB Dongle
    // turn on LED and BUTTON
    P1DIR |= 3;
    // Activate BUTTON - Do we need this?
    //CC1111EM_BUTTON = 1;

#else
    // CC1111 USB (ala Chronos watch dongle), we just need LED
    P1DIR |= 3;

#endif      // CC1111

#endif      // conditional config


#ifndef VIRTUAL_COM
    // Turn off LED
    LED = 0;
#endif
}


void clock_init(void){
    //  SET UP CPU SPEED!  USE 26MHz for CC1110 and 24MHz for CC1111
    // Set the system clock source to HS XOSC and max CPU speed,
    // ref. [clk]=>[clk_xosc.c]
    SLEEP &= ~SLEEP_OSC_PD;
    while( !(SLEEP & SLEEP_XOSC_S) );
    CLKCON = (CLKCON & ~(CLKCON_CLKSPD | CLKCON_OSC)) | CLKSPD_DIV_1;
    while (CLKCON & CLKCON_OSC);
    SLEEP |= SLEEP_OSC_PD;
    while (!IS_XOSC_STABLE());
}

/*************************************************************************************************
 * main startup code                                                                             *
 *************************************************************************************************/
void initBoard(void)
{
    clock_init();
    io_init();
}


void main (void)
{
    initBoard();
#ifdef VIRTUAL_COM
    vcom_init();
#else
    initUSB();
#endif
    init_RF();

#ifdef VIRTUAL_COM
    vcom_up();

    /* Make sure interrupts are enabled */
    EA = 1;
#endif

    appMainInit();

    while (1)
    {  
#ifndef VIRTUAL_COM
        usbProcessEvents();
#endif
        appMainLoop();
    }

}

