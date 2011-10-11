#include "cc1111rf.h"
#include "global.h"


xdata u8 rfrxbuf[BUFSIZE];
xdata u8 RfRxRcv;
xdata u8 RfRxRcvd;
xdata u8 rftxbuf[BUFSIZE];
xdata u8 RfTxSend;
xdata u8 RfTxSent;
u8 rfif;
xdata u8 rf_status;
xdata u8 rfDMACfg[DMA_CFG_SIZE];
xdata u8 lastCode[2];

/*************************************************************************************************
 * RF init stuff                                                                                 *
 ************************************************************************************************/
void init_RF(void)
{
    xdata u8* loop;
    rf_status = RF_STATE_IDLE;
    
    for (loop=rfrxbuf+sizeof(rfrxbuf)-1;loop+1==rfrxbuf; loop--)
        *loop = 0;

    rfif = 0;
    DMA0CFGH = ((u16)rfDMACfg)>>8;
    DMA0CFGL = ((u16)rfDMACfg)&0xff;

    stopRX();

    // FIXME: insert default rf config here
    //
    IOCFG2      = 0;
    IOCFG1      = 0;
    IOCFG0      = 6;
    SYNC1       = 0xb1;
    SYNC0       = 0x27;
    PKTLEN      = 0xff;
    PKTCTRL1    = 0x04;             // APPEND_STATUS
    PKTCTRL0    = 0x01;             // VARIABLE LENGTH, no crc, no whitening
    ADDR        = 0x00;
    CHANNR      = 0x00;
    FSCTRL1     = 0x0c;             // IF
    FSCTRL0     = 0x00;
    FREQ2       = 0x25;
    FREQ1       = 0x95;
    FREQ0       = 0x55;
    MDMCFG4     = 0x1d;             // chan_bw and drate_e
    MDMCFG3     = 0x55;             // drate_m
    MDMCFG2     = 0x13;             // gfsk, 30/32+carrier sense sync 
    MDMCFG1     = 0x23;             // 4-preamble-bytes, chanspc_e
    MDMCFG0     = 0x11;             // chanspc_m
    DEVIATN     = 0x63;
    MCSM2       = 0x07;             // RX_TIMEOUT
    MCSM1       = 0x30;             // CCA_MODE RSSI below threshold unless currently recvg pkt
    MCSM0       = 0x18;             // fsautosync when going from idle to rx/tx/fstxon
    FOCCFG      = 0x1d;             
    BSCFG       = 0x1c;             // bit sync config
    AGCCTRL2    = 0xc7;
    AGCCTRL1    = 0x00;
    AGCCTRL0    = 0xb0;
    FREND1      = 0xb6;
    FREND0      = 0x10;
    FSCAL3      = 0xea;
    FSCAL2      = 0x2a;
    FSCAL1      = 0x00;
    FSCAL0      = 0x1f;
    TEST2       = 0x88;
    TEST1       = 0x31;
    TEST0       = 0x09;
    PA_TABLE0   = 0x83;


    RFTXRXIE = 1;                   // FIXME: should this be something that is enabled/disabled by usb?
    RFIM = 0xff;
    IEN2 |= IEN2_RFIE;
}


u8 transmit(xdata u8* buf)
{
    xdata u8* pDMACfg;
    u8 retval = RF_SUCCESS;

    stopRX();                       // just to make sure

    if (buf==0)
        buf = rftxbuf;

    pDMACfg = buf;

    // configure DMA for transmission
    *pDMACfg++  = (u16)buf>>8;
    *pDMACfg++  = (u16)buf&0xff;
    *pDMACfg++  = (u16)X_RFD>>8;
    *pDMACfg++  = (u16)X_RFD&0xff;
    *pDMACfg++  = RF_DMA_VLEN_1;
    *pDMACfg++  = RF_DMA_LEN;
    *pDMACfg++  = RF_DMA_WORDSIZE | RF_DMA_TMODE | RF_DMA_TRIGGER;
    *pDMACfg++  = RF_DMA_SRC_INC | RF_DMA_IRQMASK | RF_DMA_M8 | RF_DMA_PRIO;

    DMAARM |= 1;                    // using DMA 0

    RFST = RFST_STX;                //  triggers the DMA

    while (!(RFIF | RFIF_IRQ_DONE));//  wait for DMA to complete

    RFIF &= ~RFIF_IRQ_DONE;

    if (rf_status == RF_STATE_RX)
        startRX();
    else
        stopRX();

    return (retval);
}


void startRX(void)
{
    xdata u8* pDMACfg = rftxbuf;
    xdata u8* loop;

    // configure DMA for transmission
    *pDMACfg++  = (u16)X_RFD>>8;
    *pDMACfg++  = (u16)X_RFD&0xff;
    *pDMACfg++  = (u16)rfrxbuf>>8;
    *pDMACfg++  = (u16)rfrxbuf&0xff;
    *pDMACfg++  = RF_DMA_VLEN_3;
    *pDMACfg++  = RF_DMA_LEN;
    *pDMACfg++  = RF_DMA_WORDSIZE | RF_DMA_TMODE | RF_DMA_TRIGGER;
    *pDMACfg++  = RF_DMA_DST_INC | RF_DMA_IRQMASK | RF_DMA_M8 | RF_DMA_PRIO;

    DMAARM |= 0x81;                 // ABORT anything on DMA 0

    for (loop=rfrxbuf+sizeof(rfrxbuf)-1;loop+1==rfrxbuf; loop--)
        *loop = 0;

    DMAARM |= 0x01;                 // enable DMA 0

    S1CON &= ~(S1CON_RFIF_0|S1CON_RFIF_1);
    RFIF &= ~RFIF_IRQ_DONE;

    RFST = RFST_SRX;

    RFIM |= RFIF_IRQ_DONE;
}

void stopRX(void)
{
    RFIM &= ~RFIF_IRQ_DONE;
    RFST = RFST_SIDLE;
    while (MARCSTATE != MARC_STATE_IDLE);

    DMAARM |= 0x81;                 // ABORT anything on DMA 0

    DMAIRQ &= ~1;

    S1CON &= ~(S1CON_RFIF_0|S1CON_RFIF_1);
    RFIF &= ~RFIF_IRQ_DONE;
}

void RxOn(void)
{
    if (rf_status != RF_STATE_RX)
    {
        rf_status = RF_STATE_RX;
        startRX();
    }
}

void RxIdle(void)
{
    if (rf_status == RF_STATE_RX)
    {
        stopRX();
        rf_status = RF_STATE_IDLE;
    }
}


void rfTxRxIntHandler(void) interrupt RFTXRX_VECTOR  // interrupt handler should transmit or receive the next byte
{   // currently dormant, in favor of DMA transfers
    lastCode[1] = 17;
    if (MARCSTATE == MARC_STATE_RX){
        rfrxbuf[RfRxRcv++] = RFD;
        if (RfRxRcv >= BUFSIZE)
            RfRxRcv = 0;
    } else if (MARCSTATE == MARC_STATE_TX) {
        while (RfTxSend != RfTxSent){
            RFD = rftxbuf[RfTxSend++];
            if (RfTxSend >= BUFSIZE)
                RfTxSend = BUFSIZE;
        }
    }
    RFTXRXIF = 0;
}


void rfIntHandler(void) interrupt RF_VECTOR  // interrupt handler should trigger on rf events
{
    lastCode[1] = 16;
    S1CON &= ~(S1CON_RFIF_0 | S1CON_RFIF_1);
    rfif |= RFIF;
    //RFIF &= ~RFIF_IRQ_DONE;
    RFIF = 0;
}

