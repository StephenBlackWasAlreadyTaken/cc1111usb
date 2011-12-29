#include "cc1111rf.h"
#include "global.h"
#include "FHSS.h"
#include "nic.h"
#include "string.h"

#ifdef VIRTUAL_COM
    #include "cc1111.h"
    #include "cc1111_vcom.h"
#else
    #include "cc1111usb.h"
#endif

/*************************************************************************************************
 * */


////  turn this on to enable TX of CARRIER at each hop instead of normal RX/TX
//#define DEBUG_HOPPING 1

#define APP_NIC 0x42
#define NIC_RECV 0x1
#define NIC_XMIT 0x2


// MAC layer defines
#define MAX_CHANNELS 880
#define MAX_TX_MSGS 5
#define MAX_TX_MSGLEN 41

#define DEFAULT_NUM_CHANS       83
#define DEFAULT_NUM_CHANHOPS    83

xdata u32 clock;


// MAC parameters (FIXME: make this all cc1111fhssmac.c/h?)
xdata u32 g_MAC_threshold;              // when the T2 clock as overflowed this many times, change channel
xdata u16 g_NumChannels;                // in case of multiple paths through the available channels 
xdata u16 g_NumChannelHops;             // total number of channels in pattern (>= g_MaxChannels)
xdata u16 g_curChanIdx;                 // indicates current channel index of the hopping pattern
xdata u8 g_Channels[MAX_CHANNELS];
xdata u16 g_tLastStateChange;
xdata u16 g_tLastHop;
xdata u16 g_desperatelySeeking;

xdata u16 g_NIC_ID;


xdata u8 g_txMsgQueue[MAX_TX_MSGS][MAX_TX_MSGLEN];
xdata u8 g_txMsgIdx = 0;

void PHY_set_channel(u16 chan);
void MAC_initChannels();
void MAC_begin_hopping(u16 T2_offset);
void MAC_stop_hopping(void);
void MAC_sync(u16 netID);
void MAC_set_chanidx(u16 chanidx);
void MAC_tx(u8 len, u8* message);
void MAC_rx_handle(u8 len, u8* message);
u8 MAC_getNextChannel();
void t1IntHandler(void) interrupt T1_VECTOR;
void t2IntHandler(void) interrupt T2_VECTOR;
void t3IntHandler(void) interrupt T3_VECTOR;

/**************************** PHY LAYER *****************************/

void PHY_set_channel(u16 chan)
{
    // set mode IDLE
    IdleMode();
    // set the channel
    CHANNR = chan;
    // if we want to transmit in this time slot, it needs to happen after a minimum delay
    RxMode();
}




/**************************** MAC LAYER *****************************/
void MAC_initChannels()
{
    // rudimentary channel setup.  this is for default hopping and testing.
    int loop;
    for (loop=0; loop<g_NumChannelHops; loop++)
    {
        g_Channels[loop] = loop % g_NumChannels;
    }
    g_MAC_threshold = 0;
}

void MAC_begin_hopping(u16 T2_offset)
{
    // reset the T2 clock settings based on T1 clock an offset
    T2CT += T2_offset;
    // start the T2 clock interrupt
    T2CTL |= T2CTL_INT;
    T2IE = 1;
    
}

void MAC_stop_hopping(void)
{
    // disable T2 interrupt
    T2CTL &= ~T2CTL_INT;
    // stop the T2 clock  - really?  nah...
}

void MAC_sync(u16 CellID)
{
    // this should be implemented for a specific MAC/PHY.  too many details are left out here.
    // what are we synching to?  need to determine if we have a network id to sync with?
    // wait on a channel until MAX_SYNC_TIMEOUT
    //
    // do we want to check current state?  this should probably only be allowed from
    // UNSYNCHED or DISCOVERY...
    if (appstatus != FHSS_STATE_UNSYNCHED && appstatus != FHSS_STATE_DISCOVERY)
    {
        debug("FHSS state entering SYNCHING from wrong state");
        debughex(appstatus);
    }
    //
    // first disable hopping 
    MAC_stop_hopping();

    // FIXME: what happens if the first channel is jammed?  make this random or make it try several
    g_curChanIdx = 0;
    MAC_set_chanidx(g_curChanIdx);

    // set state =  SYNC
    appstatus = FHSS_STATE_SYNCHING;

    // store the main timer value for beginning of this phase.
    g_tLastStateChange = clock;

    // store the cell we're seeking.  since this search will use other parts of the code...
    g_desperatelySeeking = CellID;

    // at MAX_SYNC_TIMEOUT,start activesync, where i become the cell master/time master, and periodically transmit beacons.
}

void MAC_stop_sync()
{
    // this only stops the hunt.  hopping is not re-enabled.  if you want that, use a different mode
    appstatus = FHSS_STATE_UNSYNCHED;
    g_tLastStateChange = clock;

}

void MAC_become_master()
{
    // this will force our nic to become the master
    appstatus = FHSS_STATE_SYNC_MASTER;
    g_tLastStateChange = clock;

}

void MAC_do_Master_scanny_thingy()
{
    appstatus = FHSS_STATE_SYNCHINGMASTER;
    g_tLastStateChange = clock;
}


void MAC_set_chanidx(u16 chanidx)
{
    PHY_set_channel( g_Channels[ chanidx ] );
}

void MAC_tx(u8 len, u8* message)
{
    // FIXME: possibly integrate USB/RF buffers so we don't have to keep copying...
    // queue data for sending at subsequent time slots.

    if (g_txMsgIdx++ >= MAX_TX_MSGS)
    {
        g_txMsgIdx = 0;
    }

    g_txMsgQueue[g_txMsgIdx][0] = len;
    memcpy(&g_txMsgQueue[g_txMsgIdx][1], message, len);
}


void MAC_set_NIC_ID(u16 NIC_ID)
{
    // this function is a placeholder for more functionality, if it makes sense... perhaps cut it.
    g_NIC_ID = NIC_ID;
}

void MAC_rx_handle(u8 len, u8* message)
{
    // does this even exist?  we should just handle received packets same as always.
    // actually, for some systems, this should send back an ACK or NACK on the same channel
}


u8 MAC_getNextChannel()
{
    g_curChanIdx++;
    if (g_curChanIdx >= MAX_CHANNELS)
    {
        g_curChanIdx = 0;
    }
    return g_Channels[g_curChanIdx];
}




/************************** Timer Interrupt Vectors **************************/
void t1IntHandler(void) interrupt T1_VECTOR  // interrupt handler should trigger on T2 overflow
{   
    clock ++;
}

void t2IntHandler(void) interrupt T2_VECTOR  // interrupt handler should trigger on T2 overflow
{
    // timer2 controls hopping.
    // if the system is not supposed to be hopping, T2 Interrupt should be disabled
    // otherwise....
    //
    // if we are here, the T2CT must have cycled.  increment rf_MAC_timer
    if (++rf_MAC_timer >= g_MAC_threshold)
    {
        // change to the next channel
        g_tLastHop = T2CT | (rf_MAC_timer<<8);
        
        if (++g_curChanIdx >= g_NumChannelHops)
        {
            g_curChanIdx = 0;
        }

        MAC_set_chanidx(g_curChanIdx);
#ifdef DEBUG_HOPPING
        RFST = RFST_SIDLE;
        while(!(MARCSTATE & MARC_STATE_IDLE));
        RFST = RFST_STX;        // for debugging purposes, we'll just transmit carrier at each hop
        LED = !LED;
        while(!(MARCSTATE & MARC_STATE_TX));
#endif
        rf_MAC_timer = 0;
    }
    // if the queue is not empty, wait but then tx.
}

void t3IntHandler(void) interrupt T3_VECTOR
{
    // transmit one message from queue... possibly more, if time allows
    // must check the time left when tx completes
}

init_FHSS(void)
{
    g_txMsgIdx = 0;
    g_curChanIdx = 0;
    g_NumChannels = DEFAULT_NUM_CHANS;
    g_NumChannelHops = DEFAULT_NUM_CHANHOPS;
    g_tLastHop = 0;
    g_tLastStateChange = 0;

    MAC_initChannels();

    appstatus = FHSS_STATE_UNSYNCHED;


    // FIXME: MAKE TIMERS TUNE TO factors of 50ms cycles
    //
    // Timer Setup:
    // T1 (main clock):  1.465khz
    // T2 (MAC clock):   1
// FIXME: this should be defined so it works with 24/26mhz
    // setup TIMER 1
    // free running mode
    // time freq:
    CLKCON |= 0x38;          //(0b111000);
    T1CTL |= T1CTL_DIV_128;
    T1CTL |= T1CTL_MODE_FREERUN;
// FIXME: turn on timer interrupts for t1 and t2
    // (TIMER2 is initially setup in cc1111rf.c in init_RF())
    // setup TIMER 2
    // NOTE:
    // !!! any changes to TICKSPD will change the calculation of MAC timer speed !!!
    //
    // free running mode
    // time freq:
    T2PR = 0;
    T2CTL |= T2CTL_TIP_64;  // 64, 128, 256, 1024
    T2CTL |= T2CTL_TIG;

    // setup TIMER 3
    // free running mode
    // tick freq: 
    T3CTL |= T3CTL_START;
}

/*************************************************************************************************
 * Application Code - these first few functions are what should get overwritten for your app     *
 ************************************************************************************************/

/* appMainInit() is called *before Interrupts are enabled* for various initialization things. */
void appMainInit(void)
{
    clock = 0;

    init_FHSS();

    RxMode();
}

/* appMain is the application.  it is called every loop through main, as does the USB handler code.
 * do not block if you want USB to work.                                                           */
void appMainLoop(void)
{
    xdata u8 processbuffer;

    switch  (appstatus)
    {
        case FHSS_STATE_UNSYNCHED:
            break;
        case FHSS_STATE_DISCOVERY:
            break;
        case FHSS_STATE_SYNCHING:
            break;
        case FHSS_STATE_SYNCHED:
        case FHSS_STATE_SYNC_MASTER:
            break;
    }
    if (rfif)
    {
        lastCode[0] = 0xd;
        IEN2 &= ~IEN2_RFIE;

        if(rfif & RFIF_IRQ_DONE)
        {
            processbuffer = !rfRxCurrentBuffer;
            if(rfRxProcessed[processbuffer] == RX_UNPROCESSED)
            {   
                // we've received a packet.  deliver it.
                if (PKTCTRL0&1)
                    txdata(APP_NIC, NIC_RECV, (u8)rfrxbuf[processbuffer][0], (u8*)&rfrxbuf[processbuffer]);
                else
                    txdata(APP_NIC, NIC_RECV, PKTLEN, (u8*)&rfrxbuf[processbuffer]);

                /* Set receive buffer to processed so it can be used again */
                rfRxProcessed[processbuffer] = RX_PROCESSED;
            }
        }

        rfif = 0;
        IEN2 |= IEN2_RFIE;
    }
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
    u8 app, cmd, len;
    xdata u8 *buf;
    xdata u8 *ptr;
    u16 loop;

    app = ep5iobuf.OUTbuf[4];
    cmd = ep5iobuf.OUTbuf[5];
    buf = &ep5iobuf.OUTbuf[6];
    //len = (u16)*buf;    - original firmware
    len = (u8)*buf;         // FIXME: should we use this?  or the lower byte of OUTlen?
    buf += 2;                                               // point at the address in memory
    // ep5iobuf.OUTbuf should have the following bytes to start:  <app> <cmd> <lenlow> <lenhigh>
    // check the application
    //  then check the cmd
    //   then process the data
    switch (app)
    {
        case APP_NIC:

        switch (cmd)
        {
            case NIC_XMIT:
                // FIXME:  this needs to place buf data into the FHSS txMsgQueue
                transmit(buf, len);
                { LED=1; sleepMillis(2); LED=0; sleepMillis(1); }
                txdata(app, cmd, 1, (xdata u8*)"0");
                break;

            case NIC_SET_CHANNELS:
                g_NumChannels = (xdata u16)*buf;
                if (g_NumChannels <= MAX_CHANNELS)
                {
                    buf += 2;
                    memcpy(&g_Channels[0], buf, g_NumChannels);
                    txdata(app, cmd, 2, (u8*)&g_NumChannels);
                } else {
                    txdata(app, cmd, 8, "NO DEAL");
                }
                break;

            case NIC_NEXT_CHANNEL:
                MAC_set_chanidx(MAC_getNextChannel());
                txdata(app, cmd, 1, &g_Channels[g_curChanIdx]);
                break;

            case NIC_CHANGE_CHANNEL:
                PHY_set_channel(*buf);
                txdata(app, cmd, 1, buf);
                break;

            case NIC_START_HOPPING:
                MAC_begin_hopping(0);
                txdata(app, cmd, 1, buf);
                break;

            case NIC_STOP_HOPPING:
                MAC_stop_hopping();
                txdata(app, cmd, 1, buf);
                break;

            case NIC_SET_MAC_THRESHOLD:
                MAC_stop_hopping();
                txdata(app, cmd, 1, buf);
                break;

            case NIC_SET_ID:
                MAC_set_NIC_ID(buf);
                txdata(app, cmd, 1, buf);
                break;

            default:
                break;
        }
        break;
    }
    ep5iobuf.flags &= ~EP_OUTBUF_WRITTEN;                       // this allows the OUTbuf to be rewritten... it's saved until now.
#endif
    return 0;
}

/* in case your application cares when an OUT packet has been completely received on EP0.       */
void appHandleEP0OUTdone(void)
{
}

/* called each time a usb OUT packet is received */
void appHandleEP0OUT(void)
{
#ifndef VIRTUAL_COM
    u16 loop;
    xdata u8* dst;
    xdata u8* src;

    // we are not called with the Request header as is appHandleEP0.  this function is only called after an OUT packet has been received,
    // which triggers another usb interrupt.  the important variables from the EP0 request are stored in ep0req, ep0len, and ep0value, as
    // well as ep0iobuf.OUTlen (the actual length of ep0iobuf.OUTbuf, not just some value handed in).

    // for our purposes, we only pay attention to single-packet transfers.  in more complex firmwares, this may not be sufficient.
    switch (ep0req)
    {
        case 1:     // poke
            
            src = (xdata u8*) &ep0iobuf.OUTbuf[0];
            dst = (xdata u8*) ep0value;

            for (loop=ep0iobuf.OUTlen; loop>0; loop--)
            {
                *dst++ = *src++;
            }
            break;
    }

    // must be done with the buffer by now...
    ep0iobuf.flags &= ~EP_OUTBUF_WRITTEN;
#endif
}

/* this function is the application handler for endpoint 0.  it is called for all VENDOR type    *
 * messages.  currently it implements a simple debug, ping, and peek functionality.              *
 * data is sent back through calls to either setup_send_ep0 or setup_sendx_ep0 for xdata vars    *
 * theoretically you can process stuff without the IN-direction bit, but we've found it is better*
 * to handle OUT packets in appHandleEP0OUTdone, which is called when the last packet is complete*/
int appHandleEP0(USB_Setup_Header* pReq)
{
#ifdef VIRTUAL_COM
    pReq = 0;
#else
    if (pReq->bmRequestType & USB_BM_REQTYPE_DIRMASK)       // IN to host
    {
        switch (pReq->bRequest)
        {
            case EP0_CMD_GET_DEBUG_CODES:
                setup_send_ep0(&lastCode[0], 2);
                break;
            case EP0_CMD_GET_ADDRESS:
                setup_sendx_ep0((xdata u8*)USBADDR, 40);
                break;
            case EP0_CMD_PEEKX:
                setup_sendx_ep0((xdata u8*)pReq->wValue, pReq->wLength);
                break;
            case EP0_CMD_PING0:
                setup_send_ep0((u8*)pReq, pReq->wLength);
                break;
            case EP0_CMD_PING1:
                setup_sendx_ep0((xdata u8*)&ep0iobuf.OUTbuf[0], 16);//ep0iobuf.OUTlen);
                break;
            case EP0_CMD_RESET:
                if (strncmp((char*)&(pReq->wValue), "RSTN", 4))           // therefore, ->wValue == "RS" and ->wIndex == "TN" or no reset
                {
                    blink(300,300);
                    break;   //didn't match the signature.  must have been an accident.
                }

                // implement a RESET by trigging the watchdog timer
                WDCTL = 0x83;   // Watchdog ENABLE, Watchdog mode, 2ms until reset
        }
    }
#endif
    return 0;
}



/*************************************************************************************************
 *  here begins the initialization stuff... this shouldn't change much between firmwares or      *
 *  devices.                                                                                     *
 *************************************************************************************************/

static void appInitRf(void)
{
    IOCFG2      = 0x00;
    IOCFG1      = 0x00;
    IOCFG0      = 0x00;
    SYNC1       = 0x0c;
    SYNC0       = 0x4e;
    PKTLEN      = 0xff;
    PKTCTRL1    = 0x40; // PQT threshold  - was 0x00
    PKTCTRL0    = 0x01;
    ADDR        = 0x00;
    CHANNR      = 0x00;
    FSCTRL1     = 0x06;
    FSCTRL0     = 0x00;
    FREQ2       = 0x24;
    FREQ1       = 0x3a;
    FREQ0       = 0xf1;
    MDMCFG4     = 0xca;
    MDMCFG3     = 0xa3;
    MDMCFG2     = 0x03;
    MDMCFG1     = 0x23;
    MDMCFG0     = 0x11;
    DEVIATN     = 0x36;
    MCSM2       = 0x07;             // RX_TIMEOUT
    MCSM1       = 0x3f;             // CCA_MODE RSSI below threshold unless currently recvg pkt - always end up in RX mode
    MCSM0       = 0x18;             // fsautosync when going from idle to rx/tx/fstxon
    FOCCFG      = 0x17;
    BSCFG       = 0x6c;
    AGCCTRL2    = 0x03;
    AGCCTRL1    = 0x40;
    AGCCTRL0    = 0x91;
    FREND1      = 0x56;
    FREND0      = 0x10;
    FSCAL3      = 0xe9;
    FSCAL2      = 0x2a;
    FSCAL1      = 0x00;
    FSCAL0      = 0x1f;
    TEST2       = 0x88; // low data rates, increased sensitivity provided by 0x81- was 0x88
    TEST1       = 0x31; // always 0x31 in tx-mode, for low data rates 0x35 provides increased sensitivity - was 0x31
    TEST0       = 0x09;
    PA_TABLE0   = 0x50;


#ifndef RADIO_EU
    //PKTCTRL1    = 0x04;             // APPEND_STATUS
    //PKTCTRL1    = 0x40;             // PQT threshold
    //PKTCTRL0    = 0x01;             // VARIABLE LENGTH, no crc, no whitening
    //PKTCTRL0    = 0x00;             // FIXED LENGTH, no crc, no whitening
    FSCTRL1     = 0x0c;             // Intermediate Frequency
    //FSCTRL0     = 0x00;
    FREQ2       = 0x25;
    FREQ1       = 0x95;
    FREQ0       = 0x55;
    //MDMCFG4     = 0x1d;             // chan_bw and drate_e
    //MDMCFG3     = 0x55;             // drate_m
    //MDMCFG2     = 0x13;             // gfsk, 30/32+carrier sense sync 
    //MDMCFG1     = 0x23;             // 4-preamble-bytes, chanspc_e
    //MDMCFG0     = 0x11;             // chanspc_m
    //DEVIATN     = 0x63;
    //FOCCFG      = 0x1d;             
    //BSCFG       = 0x1c;             // bit sync config
    //AGCCTRL2    = 0xc7;
    //AGCCTRL1    = 0x00;
    //AGCCTRL0    = 0xb0;
    FREND1      = 0xb6;
    FREND0      = 0x10;
    FSCAL3      = 0xea;
    FSCAL2      = 0x2a;
    FSCAL1      = 0x00;
    FSCAL0      = 0x1f;
    //TEST2       = 0x88;
    //TEST1       = 0x31;
    //TEST0       = 0x09;
    //PA_TABLE0   = 0x83;
#endif

}

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
    initUSB();
    blink(300,300);

    init_RF();
    appMainInit();

    usb_up();

    /* Enable interrupts */
    EA = 1;

    while (1)
    {  
        usbProcessEvents();
        appMainLoop();
    }

}

