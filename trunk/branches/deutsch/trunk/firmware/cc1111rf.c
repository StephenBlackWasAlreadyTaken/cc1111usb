#include "cc1111rf.h"

#include <string.h>

/* Rx buffers */
volatile xdata u8 rfRxCurrentBuffer;
volatile xdata u8 rfrxbuf[BUFFER_AMOUNT][BUFFER_SIZE];
volatile xdata u8 rfRxCounter[BUFFER_AMOUNT];
volatile xdata u8 rfRxProcessed[BUFFER_AMOUNT];
/* Tx buffers */
volatile xdata u8 rftxbuf[BUFFER_SIZE];
volatile xdata u8 rfTxCounter = 0;

u8 rfif;
xdata u8 rf_status;
static xdata u8 rfDMACfg[DMA_CFG_SIZE];
xdata u8 lastCode[2];

/*************************************************************************************************
 * RF init stuff                                                                                 *
 ************************************************************************************************/
void init_RF(void)
{
    /* clear buffers */
    memset(rfrxbuf,0,(BUFFER_AMOUNT * BUFFER_SIZE));
    memset(rftxbuf,0,BUFFER_SIZE);

    // FIXME: insert default rf config here
    //
    /*IOCFG2      = 0;
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
    PA_TABLE0   = 0x83;*/

	/* Setup interrupts */
	RFTXRXIE = 1;                   // FIXME: should this be something that is enabled/disabled by usb?
	RFIM = 0xff;
	RFIF = 0;
	rfif = 0;
	IEN2 |= IEN2_RFIE;

	/* Put radio into idle state */
	setRFIdle();
}

void setRFIdle(void)
{
	RFST = RFST_SIDLE;
	while(!(MARCSTATE & MARC_STATE_IDLE));
	rf_status = RF_STATE_IDLE;
}

u8 transmit(xdata u8* buf)
{
	/* Put radio into idle state */
	setRFIdle();

	/* Clean tx buffer */
	memset(rftxbuf,0,BUFFER_SIZE);

	/* Copy userdata to tx buffer */
	strcpy(rftxbuf,buf);

	/* Reset byte pointer */
	rfTxCounter = 0;

	/* Put radio into tx state */
	RFST = RFST_STX;
	while(!(MARCSTATE & MARC_STATE_TX));

	return 0;
}

void startRX(void)
{
    memset(rfrxbuf,0,BUFFER_SIZE);

    /* Set both byte counters to zero */
	rfRxCounter[FIRST_BUFFER] = 0;
	rfRxCounter[SECOND_BUFFER] = 0;

	/*
	 * Process flags, set first flag to false in order to let the ISR write bytes into the buffer,
	 *  The second buffer should flag processed on initialize because it is empty.
	 */
	rfRxProcessed[FIRST_BUFFER] = RX_UNPROCESSED;
	rfRxProcessed[SECOND_BUFFER] = RX_PROCESSED;

	/* Set first buffer as current buffer */
	rfRxCurrentBuffer = 0;

	S1CON &= ~(S1CON_RFIF_0|S1CON_RFIF_1);
	RFIF &= ~RFIF_IRQ_DONE;

	RFST = RFST_SRX;

	RFIM |= RFIF_IRQ_DONE;
}

void stopRX(void)
{
    RFIM &= ~RFIF_IRQ_DONE;
    setRFIdle();

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
{
    lastCode[1] = 17;

    if(MARCSTATE == MARC_STATE_RX)
    {
    	rfrxbuf[rfRxCurrentBuffer][rfRxCounter[rfRxCurrentBuffer]++] = RFD;
        if(rfRxCounter[rfRxCurrentBuffer] >= BUFFER_SIZE)
        {
        	rfRxCounter[rfRxCurrentBuffer] = 0;
        }
    }
    else if(MARCSTATE == MARC_STATE_TX)
    {
    	if(rftxbuf[rfTxCounter] != 0)
    	{
			RFD = rftxbuf[rfTxCounter++];
    	}
    }
    RFTXRXIF = 0;
}


void rfIntHandler(void) interrupt RF_VECTOR  // interrupt handler should trigger on rf events
{
    lastCode[1] = 16;
    S1CON &= ~(S1CON_RFIF_0 | S1CON_RFIF_1);
    rfif |= RFIF;

    if(RFIF & RFIF_IRQ_RXOVF)
    {
    	//P1_3 = 1;

    	/* RX overflow, only way to get out of this is to restart receiver */
    	stopRX();
    	startRX();
    }
    else if(RFIF & RFIF_IRQ_TXUNF)
    {
    	//P1_3 = 1;

    	/* Put radio into idle state */
		setRFIdle();
    }
    else if(RFIF & RFIF_IRQ_DONE)
    {
    	//P2_4 ? (P2_4 = 0) : (P2_4 = 1);
    	if(rfRxProcessed[!rfRxCurrentBuffer] == RX_PROCESSED)
    	{
    		/* Clear processed buffer */
    		memset(rfrxbuf[!rfRxCurrentBuffer],0,BUFFER_SIZE);
    		/* Switch current buffer */
    		rfRxCurrentBuffer ^= 1;
    		rfRxCounter[rfRxCurrentBuffer] = 0;
    		/* Set both buffers to unprocessed */
    		rfRxProcessed[FIRST_BUFFER] = RX_UNPROCESSED;
    		rfRxProcessed[SECOND_BUFFER] = RX_UNPROCESSED;
    	}
    	else
    	{
    		/* Main app didn't process previous packet yet, drop this one */
    		memset(rfrxbuf[rfRxCurrentBuffer],0,BUFFER_SIZE);
    		rfRxCounter[rfRxCurrentBuffer] = 0;
    	}
    }
    //RFIF &= ~RFIF_IRQ_DONE;
    RFIF = 0;
}
