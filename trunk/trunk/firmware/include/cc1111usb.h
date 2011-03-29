#define CC1111USB

#include "cc1111.h"

// only pertinent for not BUSYBLINK
#define     MAX_BLINK_QUEUE  50

#define     EP0_MAX_PACKET_SIZE     64
#define     EP5OUT_MAX_PACKET_SIZE  64
#define     EP5IN_MAX_PACKET_SIZE   500
//   #define     EP5_MAX_PACKET_SIZE     255
        // note: descriptor needs to be adjusted to match EP5_MAX_PACKET_SIZE

#ifdef IMMEDONGLE
    #define LED_RED   P2_3
    #define LED_GREEN P2_4
    #define SLEEPTIMER  1200
    
#elif defined DONSDONGLES
    // CC1111 USB Dongle
    #define LED_RED   P1_1
    #define LED_GREEN P1_1
    #define SLEEPTIMER  1200
    #define CC1111EM_BUTTON P1_2

#else
    // CC1111 USB (ala Chronos watch dongle), we just need LED
    #define LED_RED   P1_0
    #define LED_GREEN P1_0
    #define SLEEPTIMER  1200
#endif

#define LED     LED_GREEN

#define DMA_CFG_SIZE 8

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
#define RF_DMA_DST_INC  1<<6
#define RF_DMA_SRC_INC  1<<4
#define RF_DMA_IRQMASK  0
#define RF_DMA_M8       0
#define RF_DMA_PRIO     1


typedef struct {
    u16 index;
    u16 endindex;
    u16 queue[MAX_BLINK_QUEUE];
} BLINK_STATE;

typedef struct {
    u8   usbstatus;
    u16  event;
    u8   config;
} USB_STATE;

typedef struct {
    u8*  INbuf;
    u16  INbytesleft;
    u8*  OUTbuf;
    u16  OUTlen;
    u16  BUFmaxlen;
    volatile u8   flags;
    u8   epstatus;
    //xdata u8*  reg;
    //void*   OUTDONE_handle;                                     // this is a function pointer which is called when the OUT transfer is done.  i may destroy this.
} USB_EP_IO_BUF;

// extern global variables
#define BUFSIZE 256
extern USB_STATE usb_data;
extern xdata u8  usb_ep0_OUTbuf[EP0_MAX_PACKET_SIZE];                  // these get pointed to by the above structure
extern xdata u8  usb_ep5_OUTbuf[EP5OUT_MAX_PACKET_SIZE];               // these get pointed to by the above structure
extern xdata USB_EP_IO_BUF     ep0iobuf;
extern xdata USB_EP_IO_BUF     ep5iobuf;
extern xdata u8 appstatus;
extern xdata u8 lastCode[2];
extern u8 rfif;
extern xdata u8 rfDMACfg[DMA_CFG_SIZE];
extern xdata u8 rfrxbuf[BUFSIZE];
extern xdata u8 RfRxRcv;
extern xdata u8 RfRxRcvd;
extern xdata u8 rftxbuf[BUFSIZE];
extern xdata u8 RfTxSend;
extern xdata u8 RfTxSent;


// provided by cc1111usb.c
void usbIntHandler(void) interrupt P2INT_VECTOR;
void p0IntHandler(void) interrupt P0INT_VECTOR;
void rfTxRxIntHandler(void) interrupt RFTXRX_VECTOR; // interrupt handler should transmit or receive the next byte
void rfIntHandler(void) interrupt RF_VECTOR; // interrupt handler should trigger on rf events
void clock_init(void);
void txdata(u8 app, u8 cmd, u16 len, u8* dataptr);
void debugEP0Req(u8 *pReq);
void debug(code u8* text);
void debughex(xdata u8 num);
void debughex16(xdata u16 num);
void debughex32(xdata u32 num);
int setup_send_ep0(u8* payload, u16 length);
int setup_sendx_ep0(xdata u8* payload, u16 length);
u16 usb_recv_ep0OUT();
//int setup_recv_ep0();
void RxOn(void);
void RxIdle(void);
u8 transmit(xdata u8*);
void stopRX(void);
void startRX(void);

//int setup_send_ep(USB_EP_IO_BUF* iobuf, u8 *payload, u16 length);
u16 usb_recv_epOUT(u8 epnum, USB_EP_IO_BUF* epiobuf);
// export as these should be called from main() during initialization.
void initUSB(void);
void waitForUSBsetup();
// export as this *must* be in main loop.
void usbProcessEvents(void);


// provided by user application
void appHandleEP0OUTdone(void);
int appHandleEP0(USB_Setup_Header* pReq);
int appHandleEP5();
void init_RF(void);
void initBoard(void);



#define EP_INBUF_WRITTEN        1
#define EP_OUTBUF_WRITTEN       2


// usb_data bits
#define USBD_CIF_SUSPEND        (u16)0x1
#define USBD_CIF_RESUME         (u16)0x2
#define USBD_CIF_RESET          (u16)0x4
#define USBD_CIF_SOFIF          (u16)0x8
#define USBD_IIF_EP0IF          (u16)0x10
#define USBD_IIF_INEP1IF        (u16)0x20
#define USBD_IIF_INEP2IF        (u16)0x40
#define USBD_IIF_INEP3IF        (u16)0x80
#define USBD_IIF_INEP4IF        (u16)0x100
#define USBD_IIF_INEP5IF        (u16)0x200
#define USBD_OIF_OUTEP1IF       (u16)0x400
#define USBD_OIF_OUTEP2IF       (u16)0x800
#define USBD_OIF_OUTEP3IF       (u16)0x1000
#define USBD_OIF_OUTEP4IF       (u16)0x2000
#define USBD_OIF_OUTEP5IF       (u16)0x4000

#define TXDATA_MAX_WAIT         100




//xdata USB_Device_Desc           descDevice;
//xdata USB_Config_Desc           descConfig;
//xdata USB_Interface_Desc        descIntf;
//xdata USB_Endpoint_Desc         descEpIN;
//xdata USB_Endpoint_Desc         descEpOUT;
//xdata USB_LANGID_Desc           descLANGID;
//xdata USB_String_Desc           descStr1;

// setup Config Descriptor  (see cc1111.h for defaults and fields to change)
// all numbers are lsb.  modify this for your own use.
void USBDESCBEGIN(void){
__asm
0001$:    ; Device descriptor
               .DB 0002$ - 0001$     ; bLength 
               .DB USB_DESC_DEVICE   ; bDescriptorType
               .DB 0x00, 0x02        ; bcdUSB
               .DB 0x02              ; bDeviceClass i
               .DB 0x00              ; bDeviceSubClass
               .DB 0x00              ; bDeviceProtocol
               .DB EP0_MAX_PACKET_SIZE ;   EP0_PACKET_SIZE
               .DB 0x51, 0x04        ; idVendor Texas Instruments
               .DB 0x15, 0x47        ; idProduct CC1111
               .DB 0x01, 0x00        ; bcdDevice             (change to hardware version)
               .DB 0x01              ; iManufacturer
               .DB 0x02              ; iProduct
               .DB 0x03              ; iSerialNumber
               .DB 0x01              ; bNumConfigurations
0002$:     ; Configuration descriptor
               .DB 0003$ - 0002$     ; bLength
               .DB USB_DESC_CONFIG   ; bDescriptorType
               .DB 0006$ - 0002$     ; 
               .DB 00
               .DB 0x01              ; NumInterfaces
               .DB 0x01              ; bConfigurationValue  - should be nonzero
               .DB 0x00              ; iConfiguration
               .DB 0x80              ; bmAttributes
               .DB 0xfa              ; MaxPower
0003$: ; Interface descriptor
               .DB 0004$ - 0003$           ; bLength
               .DB USB_DESC_INTERFACE      ; bDescriptorType
               .DB 0x00                    ; bInterfaceNumber
               .DB 0x00                    ; bAlternateSetting
               .DB 0x02                    ; bNumEndpoints
               .DB 0xff                    ; bInterfaceClass
               .DB 0xff                    ; bInterfaceSubClass
               .DB 0x01                    ; bInterfaceProcotol
               .DB 0x00                    ; iInterface
0004$:  ; Endpoint descriptor (EP5 IN)
               .DB 0005$ - 0004$           ; bLength
               .DB USB_DESC_ENDPOINT       ; bDescriptorType
               .DB 0x85                    ; bEndpointAddress
               .DB 0x02                    ; bmAttributes - bits 0-1 Xfer Type (0=Ctrl, 1=Isoc, 2=Bulk, 3=Intrpt);      2-3 Isoc-SyncType (0=None, 1=FeedbackEndpoint, 2=Adaptive, 3=Synchronous);       4-5 Isoc-UsageType (0=Data, 1=Feedback, 2=Explicit)
               .DB 0xf4, 0x01              ; wMaxPacketSize
               .DB 0x01                    ; bInterval
0005$:  ; Endpoint descriptor (EP5 OUT)
               .DB 0006$ - 0005$           ; bLength
               .DB USB_DESC_ENDPOINT       ; bDescriptorType
               .DB 0x05                    ; bEndpointAddress
               .DB 0x02                    ; bmAttributes
               .DB 0x40, 0x00              ; wMaxPacketSize
               .DB 0x01                    ; bInterval
0006$:    ; Language ID
               .DB 0007$ - 0006$           ; bLength
               .DB USB_DESC_STRING         ; bDescriptorType
               .DB 0x09                    ; US-EN
               .DB 0x04
0007$:    ; Manufacturer
               .DB 0008$ - 0007$           ; bLength
               .DB USB_DESC_STRING         ; bDescriptorType
               .DB "a", 0
               .DB "t", 0
               .DB "l", 0
               .DB "a", 0
               .DB "s", 0
               .DB " ", 0
               .DB "i", 0
               .DB "n", 0
               .DB "s", 0
               .DB "t", 0
               .DB "r", 0
               .DB "u", 0
               .DB "m", 0
               .DB "e", 0
               .DB "n", 0
               .DB "t", 0
               .DB "s", 0
0008$:    ; Product
               .DB 0009$ - 0008$             ; bLength
               .DB USB_DESC_STRING           ; bDescriptorType
               .DB "C", 0
               .DB "C", 0
               .DB "1", 0
               .DB "1", 0
               .DB "1", 0
               .DB "1", 0
               .DB " ", 0
               .DB "U", 0
               .DB "S", 0
               .DB "B", 0
               .DB " ", 0
               .DB "K", 0
               .DB "i", 0
               .DB "c", 0
               .DB "k", 0
               .DB "a", 0
               .DB "s", 0
               .DB "s", 0
0009$:   ;; Serial number
               .DB 0010$ - 0009$            ;; bLength
               .DB USB_DESC_STRING          ;; bDescriptorType
               .DB "0", 0
               .DB "0", 0
               .DB "1", 0
0010$:
               .DB  0
               .DB  0xff
__endasm;
}

//! Sleep for some milliseconds.
void sleepMillis(int ms) {
    int j;
    while (--ms > 0) { 
        for (j=0; j<SLEEPTIMER;j++); // about 1 millisecond
    };
}


void sleepMicros(int us) {
    while (--us > 0) { 
    };
}



#define BUSYBLINK 


#ifdef BUSYBLINK

#define REALLYFASTBLINK()        { LED=1; sleepMillis(2); LED=0; sleepMillis(10); }
/// #define blink( on_cycles, off_cycles)  {LED=1; sleepMillis(on_cycles); LED=0; sleepMillis(off_cycles);}
void blink(u16 on_cycles, u16 off_cycles)                    // haxed for memory usage... made define instead
{
    LED=1;
    sleepMillis(on_cycles);
    LED=0;
    sleepMillis(off_cycles);
}

void blink_binary_baby_lsb(u16 num, char bits)
{
    EA=0;
    LED = 1;
    sleepMillis(1000);
    LED = 0;
    sleepMillis(500);
    bits -= 1;          // 16 bit numbers needs to start on bit 15, etc....

    for (; bits>=0; bits--)
    {
        if (num & 1)
        {
            sleepMillis(25);
            LED = 1;
            sleepMillis(550);
            LED = 0;
            sleepMillis(25);
        }
        else
        {
            sleepMillis(275);
            LED = 1;
            sleepMillis(50);
            LED = 0;
            sleepMillis(275);
        }
        num = num >> 1;
    }
    LED = 0;
    sleepMillis(1000);
    EA=1;
}

/*
void blink_binary_baby_msb(u16 num, char bits)
{
    LED = 1;
    sleepMillis(1500);
    LED = 0;
    sleepMillis(100);
    bits -= 1;          // 16 bit numbers needs to start on bit 15, etc....

    for (; bits>=0; bits--)
    {
        if (num & (1<<bits))
        {
            LED = 0;
            sleepMillis(10);
            LED = 1;
        }
        else
        {
            LED = 1;
            sleepMillis(10);
            LED = 0;
        }
        sleepMillis(350);
    }
    LED = 0;
    sleepMillis(1500);
}*/
#else

#define REALLYFASTBLINK()       blink(20,100);
void blink(u16 on_cycles, u16 off_cycles){
    u8 tEA= EA;                                  // store Interrupt State
    u8 ei;
    EA=0;                                           // disable Interrupts
    ei = blinkstate.endindex + 2;
    if (ei == MAX_BLINK_QUEUE)
        ei = 0;
    blinkstate.endindex = ei;                       // storing on and off delay
    blinkstate.queue[ei] = on_cycles;
    blinkstate.queue[ei+1] = off_cycles;
    EA=tEA;                                         // if Interrupts *were* on, turn them back on... not so beforehand.
}


void do_blink()
{
    u16 counter = --blinkstate.queue[blinkstate.index];

    if (!counter)
    {
        if (blinkstate.index < blinkstate.endindex)
        {
            blinkstate.index++;
            LED = ~LED;
        }

    }
}

void blink_binary_baby_lsb(u16 num, char bits)
{
    EA=0;
    LED = 1;
    sleepMillis(1000);
    LED = 0;
    sleepMillis(500);
    bits -= 1;          // 16 bit numbers needs to start on bit 15, etc....

    for (; bits>=0; bits--)
    {
        if (num & 1)
        {
            blink(1,25);
            blink(550,25);
//            sleepMillis(25);
//            LED = 1;
//            sleepMillis(550);
//            LED = 0;
//            sleepMillis(25);
        }
        else
        {
            blink(1,275);
            blink(50,275);
//            sleepMillis(275);
//            LED = 1;
//            sleepMillis(50);
//            LED = 0;
//            sleepMillis(275);
        }
        num = num >> 1;
    }
    LED = 0;
    sleepMillis(1000);
    EA=1;
}

#endif                      // NOREALTIMEBLINK

#define     CMD_PEEK        0x80
#define     CMD_POKE        0x81
#define     CMD_PING        0x82
#define     CMD_STATUS      0x83
#define     CMD_POKE_REG    0x84
#define     CMD_RFMODE      0x85

#define     DEBUG_CMD_STRING    0xf0
#define     DEBUG_CMD_HEX       0xf1
#define     DEBUG_CMD_HEX16     0xf2
#define     DEBUG_CMD_HEX32     0xf3
#define     DEBUG_CMD_INT       0xf4
