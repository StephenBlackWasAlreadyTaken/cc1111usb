#include "cc1111rf.h"
#include "global.h"

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
volatile xdata u8 rf_status;
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

    IOCFG2      = 0x00;
	IOCFG1      = 0x00;
	IOCFG0      = 0x00;
	SYNC1       = 0x0c;
	SYNC0       = 0x4e;
	PKTLEN      = 0xff;
#ifdef RECEIVE_TEST
	PKTCTRL1    = 0x40;
#else
	PKTCTRL1	= 0x00;
#endif
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
	MCSM2       = 0x07;
#ifdef RECEIVE_TEST
	MCSM1       = 0x3C;
#else
	MCSM1       = 0x30;
#endif
	MCSM0       = 0x18;
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
#ifdef RECEIVE_TEST
	TEST2       = 0x81;
	TEST1       = 0x35;
#else
	TEST2       = 0x88;
	TEST1       = 0x31;
#endif
	TEST0       = 0x09;
	PA_TABLE0   = 0x50;

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

int waitRSSI()
{
	u16 u16WaitTime = 0;
	while(u16WaitTime < RSSI_TIMEOUT_US)
	{
		if(PKTSTATUS & (PKTSTATUS_CCA | PKTSTATUS_CS))
		{
			return 1;
		}
		else
		{
			sleepMicros(50);
			u16WaitTime += 50;
		}
	}
	return 0;
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

	/* Strobe to rx */
	RFST = RFST_SRX;
	while(!(MARCSTATE & MARC_STATE_RX));
	/* wait for good RSSI */
	while(1)
	{
		if(PKTSTATUS & (PKTSTATUS_CCA | PKTSTATUS_CS))
		{
			break;
		}
	}

	/* Put radio into tx state */
	RFST = RFST_STX;
	while(!(MARCSTATE & MARC_STATE_TX));

//	if(waitRSSI)
//	{
//		/* Put radio into tx state */
//		RFST = RFST_STX;
//		while(!(MARCSTATE & MARC_STATE_TX));
//	}
//	else
//	{
//		/* failed, retry? */
//		P1_3 = 1;
//	}
	return 0;
}

//u8 transmitdma(xdata u8* buf)
//{
//	u8 retval = RF_SUCCESS;
//
//	setRFIdle();
//
//	rf_status = RF_STATE_TX;
//
//	// configure DMA for transmission
//	rfDMACfg[0]  = (u16)buf>>8;
//	rfDMACfg[1]  = (u16)buf&0xff;
//	rfDMACfg[2]  = (u16)X_RFD>>8;
//	rfDMACfg[3]  = (u16)X_RFD&0xff;
//	rfDMACfg[4]  = RF_DMA_VLEN_1;
//	rfDMACfg[5]  = RF_DMA_LEN;
//	rfDMACfg[6]  = RF_DMA_TRIGGER;
//	rfDMACfg[7]  = RF_DMA_SRC_INC | RF_DMA_IRQMASK_DI | RF_DMA_M8 | RF_DMA_PRIO_NOR;
//
//	DMA0CFGH = ((u16)rfDMACfg)>>8;
//	DMA0CFGL = ((u16)rfDMACfg)&0xff;
//
//	/* Strobe to rx */
//	RFST = RFST_SRX;
//	while(!(MARCSTATE & MARC_STATE_RX));
//
//	/* wait for good RSSI */
//	while(1)
//	{
//		if(PKTSTATUS & (PKTSTATUS_CCA | PKTSTATUS_CS))
//		{
//			break;
//		}
//	}
//
//	DMAARM |= 0x01;                    // using DMA 0
//	RFST = RFST_STX;                //  triggers the DMA
//
//	/*while (!(RFIF & RFIF_IRQ_DONE));//  wait for DMA to complete
//	RFIF &= ~RFIF_IRQ_DONE;
//
//	if (rf_status == RF_STATE_RX)
//		startRX();
//	else
//		stopRX();
//	 */
//
//	return (retval);
//}

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
    	P0_3 = 1;

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
    	if(rf_status == RF_STATE_TX)
    	{
    		//P1_3 = 1;
    		DMAARM |= 0x81;
    	}
    	else
    	{
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
    }
#ifdef RECEIVE_TEST
    else if(RFIF & RFIF_IRQ_SFD)
    {
    	P0_3 ? (P0_3 = 0) : (P0_3 = 1);
    }
    else if(RFIF & RFIF_IRQ_CCA)
	{
		P0_2 ? (P0_2 = 0) : (P0_2 = 1);
	}
    else if(RFIF & RFIF_IRQ_PQT)
    {
    	P0_1 ? (P0_1 = 0) : (P0_1 = 1);
    }
    else if(RFIF & RFIF_IRQ_CS)
    {
    	P0_0 ? (P0_0 = 0) : (P0_0 = 1);
    }
#endif
    //RFIF &= ~RFIF_IRQ_DONE;
    RFIF = 0;
}
