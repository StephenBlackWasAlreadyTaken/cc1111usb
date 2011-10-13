#include "cc1111rf.h"
#include "global.h"

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
	xdata u8 *startPtr = 0;
	xdata u8 *endPtr = 0;	

    rf_status = RF_STATE_IDLE;

    //  FIXME: DMA remains
    //    DMA0CFGH = ((u16)rfDMACfg)>>8;
    //    DMA0CFGL = ((u16)rfDMACfg)&0xff;

    /* clear buffers */
	startPtr = &rfrxbuf[0][0];
	endPtr = startPtr + (BUFFER_AMOUNT * BUFFER_SIZE);
	while(startPtr < endPtr)
	{
		*startPtr++ = 0;
	}	 


#ifdef RADIO_EU
    //memset(rftxbuf,0,BUFFER_SIZE);   // unnecessary?

    IOCFG2      = 0x00;
	IOCFG1      = 0x00;
	IOCFG0      = 0x00;
	SYNC1       = 0x0c;
	SYNC0       = 0x4e;
	PKTLEN      = 0xff;
	PKTCTRL1	= 0x00;
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
	MCSM1       = 0x30;
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
	TEST2       = 0x88;
	TEST1       = 0x31;
	TEST0       = 0x09;
	PA_TABLE0   = 0x50;


#else
    // default rf config
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
#endif

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


u8 transmit(xdata u8* buf, u16 len)
{
	xdata u8 *startPtr = 0;
	xdata u8 *endPtr = 0;
	u16 i = 0;	

	/* Put radio into idle state */
	setRFIdle();

	/* Clean tx buffer */
	startPtr = rftxbuf;
	endPtr = startPtr + BUFFER_SIZE;
	while(startPtr < endPtr)
	{
		*startPtr++ = 0;
	}

	/* Copy userdata to tx buffer */
	for(i = 0; i < len; i++)
	{
		rftxbuf[i] = buf[i];
	}

	/* Reset byte pointer */
	rfTxCounter = 0;

    //pDMACfg = buf;                //wtf was i thinking here?!?

	/* Strobe to rx */
	RFST = RFST_SRX;
	while(!(MARCSTATE & MARC_STATE_RX));
	/* wait for good RSSI (clear channel) */
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

	return 0;
}


/*  FIXME: this is old code... however, it was failing may prove useful later.  this didn't work, but it was
 *  failing during the "SRC/DST_DMA_INC" bug.  may try again.
    // configure DMA for transmission
    *pDMACfg++  = (u16)buf>>8;
    *pDMACfg++  = (u16)buf&0xff;
    *pDMACfg++  = (u16)X_RFD>>8;
    *pDMACfg++  = (u16)X_RFD&0xff;
    *pDMACfg++  = RF_DMA_VLEN_1;
    *pDMACfg++  = RF_DMA_LEN;
    *pDMACfg++  = RF_DMA_WORDSIZE | RF_DMA_TMODE | RF_DMA_TRIGGER;
    *pDMACfg++  = RF_DMA_SRC_INC | RF_DMA_IRQMASK | RF_DMA_M8 | RF_DMA_PRIO_LOW;

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
*/

void startRX(void)
{
	xdata u8 *startPtr = 0;
	xdata u8 *endPtr = 0;
	
    /* clear buffers */
	startPtr = &rfrxbuf[0][0];
	endPtr = startPtr + (BUFFER_AMOUNT * BUFFER_SIZE);
	while(startPtr < endPtr)
	{
		*startPtr++ = 0;
	}	 

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
/*  FIXME: this is old code... however, it was failing may prove useful later.  this didn't work, but it was
 *  failing during the "SRC/DST_DMA_INC" bug.  may try again.
 * {
    volatile xdata u8* pDMACfg = rftxbuf;
    volatile xdata u8* loop;

    // configure DMA for transmission
    *pDMACfg++  = (u16)X_RFD>>8;
    *pDMACfg++  = (u16)X_RFD&0xff;
    *pDMACfg++  = (u16)rfrxbuf>>8;
    *pDMACfg++  = (u16)rfrxbuf&0xff;
    *pDMACfg++  = RF_DMA_VLEN_3;
    *pDMACfg++  = RF_DMA_LEN;
    *pDMACfg++  = RF_DMA_WORDSIZE | RF_DMA_TMODE | RF_DMA_TRIGGER;
    *pDMACfg++  = RF_DMA_DST_INC | RF_DMA_IRQMASK | RF_DMA_M8 | RF_DMA_PRIO_LOW;

    DMAARM |= 0x81;                 // ABORT anything on DMA 0

    loop=(volatile xdata u8*)rfrxbuf+(BUFFER_AMOUNT*BUFFER_SIZE)-1;
    for (;loop+1==&rfrxbuf[0][0]; loop--)
        *loop = 0;

    DMAARM |= 0x01;                 // enable DMA 0

    S1CON &= ~(S1CON_RFIF_0|S1CON_RFIF_1);
    RFIF &= ~RFIF_IRQ_DONE;

    RFST = RFST_SRX;

    RFIM |= RFIF_IRQ_DONE;
}*/

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
{   // currently dormant, in favor of DMA transfers
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
	xdata u8 *startPtr = 0;
	xdata u8 *endPtr = 0;
    lastCode[1] = 16;
    S1CON &= ~(S1CON_RFIF_0 | S1CON_RFIF_1);
    rfif |= RFIF;

    if(RFIF & RFIF_IRQ_RXOVF)
    {
    	//P1_3 = 1;
    	//P0_3 = 1;

        REALLYFASTBLINK();
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
                REALLYFASTBLINK();
				/* Clear processed buffer */
				startPtr = &rfrxbuf[!rfRxCurrentBuffer][0];
				endPtr = startPtr + BUFFER_SIZE;
				while(startPtr < endPtr)
				{
					*startPtr++ = 0;
				}	 
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
				startPtr = &rfrxbuf[rfRxCurrentBuffer][0];
				endPtr = startPtr + BUFFER_SIZE;
				while(startPtr < endPtr)
				{
					*startPtr++ = 0;
				}	 
				rfRxCounter[rfRxCurrentBuffer] = 0;
			}
    	}
    }

    RFIF = 0;
}

