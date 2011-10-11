#ifndef CC1111RF_H
#define CC1111RF_H

#include "cc1111.h"

#define DMA_CFG_SIZE 8
#define BUFSIZE 255

#define RF_STATE_RX 1
#define RF_STATE_TX 2
#define RF_STATE_IDLE 3

#define RF_SUCCESS 0

#define RF_DMA_VLEN_1   1<<5
#define RF_DMA_VLEN_3   4<<5
#define RF_DMA_LEN      0xfe
#define RF_DMA_WORDSIZE 1<<7
#define RF_DMA_TMODE    0
#define RF_DMA_TRIGGER  19
#define RF_DMA_DST_INC  1<<4
#define RF_DMA_SRC_INC  1<<6
#define RF_DMA_IRQMASK  0
#define RF_DMA_M8       0
#define RF_DMA_PRIO     1

extern u8 rfif;
extern xdata u8 lastCode[2];
extern xdata u8 rfDMACfg[DMA_CFG_SIZE];
extern xdata u8 rfrxbuf[BUFSIZE];
extern xdata u8 RfRxRcv;
extern xdata u8 RfRxRcvd;
extern xdata u8 rftxbuf[BUFSIZE];
extern xdata u8 RfTxSend;
extern xdata u8 RfTxSent;

void rfTxRxIntHandler(void) interrupt RFTXRX_VECTOR; // interrupt handler should transmit or receive the next byte
void rfIntHandler(void) interrupt RF_VECTOR; // interrupt handler should trigger on rf events

void RxOn(void);
void RxIdle(void);
u8 transmit(xdata u8*);
void stopRX(void);
void startRX(void);
void init_RF(void);

#endif
