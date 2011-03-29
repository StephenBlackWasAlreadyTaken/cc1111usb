#include "cc1111usb.h"


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
}

/* appMain is the application.  it is called every loop through main, as does the USB handler code.
 * please do not block if you want USB to work.                                                 */
void appMainLoop(void)
{
    if (rfif){
        lastCode[0] = 0xd;
        IEN2 &= ~IEN2_RFIE;
        debughex(rfif);
        rfif = 0;
        IEN2 |= IEN2_RFIE;
    }
    //debug("testing");
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
{
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
    return 0;

}

/* in case your application cares when an OUT packet has been completely received.               */
void appHandleEP0OUTdone(void)
{
}

/* this function is the application handler for endpoint 0.  it is called for all VENDOR type    *
 * messages.  currently it implements a simple ping-like application.                           */
int appHandleEP0(USB_Setup_Header* pReq)
{
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
    return 0;
}



/*************************************************************************************************
 *  here begins the initialization stuff... this shouldn't change much between firmwares or      *
 *  devices.                                                                                     *
 *************************************************************************************************/

/* initialize the IO subsystems for the appropriate dongles */
static void io_init(void)
{
#ifdef IMMEDONGLE
    // IM-ME Dongle.  It's a CC1110, so no USB stuffs.  Still, a bit of stuff to init for talking to it's own Cypress USB chip
    P0SEL |= (BIT5 | BIT3);     // Select SCK and MOSI as SPI
    P0DIR |= BIT4 | BIT6;       // SSEL and LED as output
    P0 &= ~(BIT4 | BIT2);       // Drive SSEL and MISO low

    P1IF = 0;                   // clear P1 interrupt flag
    IEN2 |= IEN2_P1IE;          // enable P1 interrupt
    P1IEN |= BIT1;              // enable interrupt for P1.1

    P1DIR |= BIT0;              // P1.0 as output, attention line to cypress
    P1 &= ~BIT0;                // not ready to receive
    
#elif defined DONSDONGLES
    // CC1111 USB Dongle
    // turn on LED and BUTTON
    P1DIR |= 3;
    // Turn off LED
    LED = 0;
    // Activate BUTTON - Do we need this?
    //CC1111EM_BUTTON = 1;

#else
    // CC1111 USB (ala Chronos watch dongle), we just need LED
    P1DIR |= 3;
    LED = 0;
#endif


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
    initUSB();
    init_RF();
    //sleepMillis(500);
    appMainInit();
    while (1)
    {  
#ifndef BUSYBLINK
        do_blink();
#endif
        usbProcessEvents();
        appMainLoop();
    }

}

