#include "cc1111.h"
#include "cc1111rf.h"
#include "global.h"

#include <string.h>

/* Rx buffers */
volatile xdata uint8_t rfRxCurrentBuffer;
volatile xdata uint8_t rfrxbuf[BUFFER_AMOUNT][BUFFER_SIZE];
volatile xdata uint8_t rfRxCounter[BUFFER_AMOUNT];
volatile xdata uint8_t rfRxProcessed[BUFFER_AMOUNT];
volatile xdata uint8_t bRxDMA;
/* Tx buffers */
volatile xdata uint8_t rftxbuf[BUFFER_SIZE];
volatile xdata uint8_t rfTxCounter = 0;

uint8_t rfif;
volatile xdata uint8_t rf_status;
static xdata struct cc_dma_channel rfDMA;

/*************************************************************************************************
 * RF init stuff                                                                                 *
 ************************************************************************************************/
void init_RF(uint8_t bEuRadio, register_e rRegisterType)
{
    /* Init DMA channel */
    DMAIRQ = 0;
    DMAIF = 0;
    DMA0CFGH = ((uint16_t)(&rfDMA))>>8;
    DMA0CFGL = ((uint16_t)(&rfDMA))&0xff;

    /* clear buffers */
    memset(rfrxbuf,0,(BUFFER_AMOUNT * BUFFER_SIZE));
    memset(rftxbuf,0,BUFFER_SIZE);
	
    IOCFG2      = 0x00;
	IOCFG1      = 0x00;
	IOCFG0      = 0x00;
	SYNC1       = 0x0c;
	SYNC0       = 0x4e;
	PKTLEN      = 0xff;
	if(rRegisterType == RECV)
	{
		PKTCTRL1    = 0x40;
	}
	else
	{
		PKTCTRL1	= 0x00;
	}
	PKTCTRL0    = 0x05;
	ADDR        = 0x00;
	CHANNR      = 0x00;
	FSCTRL1     = 0x08;
	FSCTRL0     = 0x00;
	FREQ2       = 0x24;
	FREQ1       = 0x2d;
	FREQ0       = 0xdd;
	MDMCFG4     = 0xca;
	MDMCFG3     = 0xa3;
	MDMCFG2     = 0x93;
	MDMCFG1     = 0x23;
	MDMCFG0     = 0x11;
	DEVIATN     = 0x36;
	MCSM2       = 0x07;
	if(rRegisterType == RECV)
	{
		MCSM1       = 0x3C;
	}
	else
	{
		MCSM1       = 0x30;
	}
	MCSM0       = 0x18;
	FOCCFG      = 0x16;
	BSCFG       = 0x6c;
	AGCCTRL2    = 0x43;
	AGCCTRL1    = 0x40;
	AGCCTRL0    = 0x91;
	FREND1      = 0x56;
	FREND0      = 0x10;
	FSCAL3      = 0xe9;
	FSCAL2      = 0x2a;
	FSCAL1      = 0x00;
	FSCAL0      = 0x1f;
	if(rRegisterType == RECV)
	{
		TEST2       = 0x81;
		TEST1       = 0x35;
	}
	else
	{
		TEST2       = 0x88;
		TEST1       = 0x31;
	}
	TEST0       = 0x09;
	PA_TABLE0   = 0x50;

	/* If not EU change frequency */
	if(!bEuRadio)
	{
		FSCTRL1 = 0x0c;
		FREQ2 = 0x25;
		FREQ1 = 0x95;
		FREQ0 = 0x55;
		FREND1 = 0xb6;
		FREND0 = 0x10;
		FSCAL3 = 0xea;
		FSCAL2 = 0x2a;
		FSCAL1 = 0x00;
		FSCAL0 = 0x1f;
	}

	/* Setup interrupts */
	RFTXRXIE = 1;                   // FIXME: should this be something that is enabled/disabled by usb?
	RFIM = 0xff;
	RFIF = 0;
	rfif = 0;
    IEN1 |= DMAIE;
	IEN2 |= IEN2_RFIE;

	/* Put radio into idle state */
	setRFIdle();
}

void setRFIdle(void)
{
	RFST = RFST_SIDLE;
	while(!(MARCSTATE & RF_MARCSTATE_IDLE));
	rf_status = RF_STATE_IDLE;
}

int waitRSSI()
{
	uint16_t u16WaitTime = 0;
	while(u16WaitTime < RSSI_TIMEOUT_US)
	{
		if(PKTSTATUS & (RF_PKTSTATUS_CCA | RF_PKTSTATUS_CS))
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

/* Functions contains attempt for DMA but not working yet, please leave bDma 0 */
uint8_t transmit(xdata uint8_t* buf, uint16_t len, uint8_t bDma)
{
	/* Put radio into idle state */
	setRFIdle();

	/* If len is empty, assume first byte is the length */
	if(len == 0)
	{
		len = buf[0];
        len++;
	}

    /* If DMA transfer, disable rxtx interrupt */
	RFTXRXIE = !bDma; 

	/* Clean tx buffer */
	memset(rftxbuf,0,len);

	/* Copy userdata to tx buffer */
	strcpy(rftxbuf,buf);

	/* Reset byte pointer */
	rfTxCounter = 0;

    /* Configure DMA struct */
    if(bDma)
    {
        rfDMA.src_high = ((uint16_t)buf)>>8;
        rfDMA.src_low = ((uint16_t)buf)&0xff;
        rfDMA.dst_high = ((uint16_t)&RFDXADDR)>>8;
        rfDMA.dst_low = ((uint16_t)&RFDXADDR)&0xff;
        rfDMA.len_high = len >> 8;
        rfDMA.len_low = len;
        rfDMA.cfg0 = DMA_CFG0_WORDSIZE_8 |
                    DMA_CFG0_TMODE_SINGLE |
                    DMA_CFG0_TRIGGER_RADIO; 
        rfDMA.cfg1 = DMA_CFG1_SRCINC_1 |
                    DMA_CFG1_DESTINC_0 |
                    DMA_CFG1_PRIORITY_HIGH;
    
        DMA0CFGH = ((uint16_t)(&rfDMA))>>8;
        DMA0CFGL = ((uint16_t)(&rfDMA))&0xff;
    }

	/* Strobe to rx */
//	RFST = RFST_SRX;
//	while(!(MARCSTATE & MARC_STATE_RX));
	/* wait for good RSSI, TODO change while loop this could hang forever */
//	while(1)
//	{
//		if(PKTSTATUS & (PKTSTATUS_CCA | PKTSTATUS_CS))
//		{
//			break;
//		}
//	}
    #define nop() _asm nop _endasm;
    
    /* Arm DMA channel */
    if(bDma)
    {
        DMAIRQ &= ~DMAARM_DMAARM0;
        DMAARM |= (0x80 | DMAARM_DMAARM0);
        nop(); nop(); nop(); nop();
        nop(); nop(); nop(); nop();
        DMAARM = DMAARM_DMAARM0;
        nop(); nop(); nop(); nop();
        nop(); nop(); nop(); nop();
        nop();
    }

	/* Put radio into tx state */
	RFST = RFST_STX;
	//while(!(MARCSTATE & RF_MARCSTATE_TX));

//	if(waitRSSI)
//	{
//		/* Put radio into tx state */
//		RFST = RFST_STX;
//		while(!(MARCSTATE & MARC_STATE_TX));
//	}
//	else
//	{
//		/* failed, retry? */
//	}
	return 0;
}

void startRX(uint8_t bDma)
{
    /* If DMA transfer, disable rxtx interrupt */
	RFTXRXIE = !bDma; 
    bRxDMA = bDma;
    
    /* Clear rx buffer */
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
	RFIF &= ~RFIF_IM_DONE;

    if(bDma)
    {

        rfDMA.src_high = ((uint16_t)&RFDXADDR)>>8;
        rfDMA.src_low = ((uint16_t)&RFDXADDR)&0xff;
        rfDMA.dst_high = ((uint16_t)&rfrxbuf[rfRxCurrentBuffer])>>8;
        rfDMA.dst_low = ((uint16_t)&rfrxbuf[rfRxCurrentBuffer])&0xff;
        rfDMA.len_high = sizeof(rfrxbuf[rfRxCurrentBuffer]) >> 8;
        rfDMA.len_low = sizeof(rfrxbuf[rfRxCurrentBuffer]); 
        rfDMA.cfg0 = DMA_CFG0_WORDSIZE_8 |
                    DMA_CFG0_TMODE_SINGLE |
                    DMA_CFG0_TRIGGER_RADIO; 
        rfDMA.cfg1 = DMA_CFG1_SRCINC_0 |
                    DMA_CFG1_DESTINC_1 |
                    DMA_CFG1_PRIORITY_HIGH;
    
        DMA0CFGH = ((uint16_t)(&rfDMA))>>8;
        DMA0CFGL = ((uint16_t)(&rfDMA))&0xff;
        
        DMAIRQ &= ~DMAARM_DMAARM0;
        DMAARM |= (0x80 | DMAARM_DMAARM0);
        nop(); nop(); nop(); nop();
        nop(); nop(); nop(); nop();
        DMAARM = DMAARM_DMAARM0;
        nop(); nop(); nop(); nop();
        nop(); nop(); nop(); nop();
    }
    /* Strobe to RX mode */
	RFST = RFST_SRX;

	RFIM |= RFIF_IM_DONE;
}

void stopRX(void)
{
    RFIM &= ~RFIF_IM_DONE;
    setRFIdle();

    DMAARM |= 0x81;                 // ABORT anything on DMA 0

    DMAIRQ &= ~1;

    S1CON &= ~(S1CON_RFIF_0|S1CON_RFIF_1);
    RFIF &= ~RFIF_IM_DONE;
}

void RxOn(void)
{
    if (rf_status != RF_STATE_RX)
    {
        rf_status = RF_STATE_RX;
        startRX(bRxDMA);
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

    if(MARCSTATE == RF_MARCSTATE_RX)
    {
    	rfrxbuf[rfRxCurrentBuffer][rfRxCounter[rfRxCurrentBuffer]++] = RFD;
    	if(rfRxCounter[rfRxCurrentBuffer] >= BUFFER_SIZE)
        {
        	rfRxCounter[rfRxCurrentBuffer] = 0;
        }
    }
    else if(MARCSTATE == RF_MARCSTATE_TX)
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

    if(RFIF & RFIF_IM_RXOVF)
    {
    	/* RX overflow, only way to get out of this is to restart receiver */
    	stopRX();
    	startRX(bRxDMA);
    }
    else if(RFIF & RFIF_IM_TXUNF)
    {
    	/* Put radio into idle state */
		setRFIdle();
    }
    else if(RFIF & RFIF_IM_DONE)
    {
    	if(rf_status == RF_STATE_TX)
    	{
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
                if(bRxDMA)
                {
                    /* Switch DMA buffer */
                    rfDMA.dst_high = ((uint16_t)&rfrxbuf[rfRxCurrentBuffer])>>8;
                    rfDMA.dst_low = ((uint16_t)&rfrxbuf[rfRxCurrentBuffer])&0xff;
                    /* Arm DMA for next receive */
                    DMAARM = DMAARM_DMAARM0;
                    nop(); nop(); nop(); nop();
                    nop(); nop(); nop(); nop();
                }
			}
			else
			{
				/* Main app didn't process previous packet yet, drop this one */
				memset(rfrxbuf[rfRxCurrentBuffer],0,BUFFER_SIZE);
				rfRxCounter[rfRxCurrentBuffer] = 0;
			}
    	}
    }
    //RFIF &= ~RFIF_IM_DONE;
    RFIF = 0;
}
