#ifndef CC1111USB
#define CC1111USB

#include "cc1111.h"
#include "global.h"

#define     EP0_MAX_PACKET_SIZE     64
#define     EP5OUT_MAX_PACKET_SIZE  64
#define     EP5IN_MAX_PACKET_SIZE   500
//   #define     EP5_MAX_PACKET_SIZE     255
        // note: descriptor needs to be adjusted to match EP5_MAX_PACKET_SIZE

#define     CMD_PEEK        0x80
#define     CMD_POKE        0x81
#define     CMD_PING        0x82
#define     CMD_STATUS      0x83
#define     CMD_POKE_REG    0x84
#define     CMD_RFMODE      0x85

#define     EP0_CMD_GET_DEBUG_CODES         0x00
#define     EP0_CMD_GET_ADDRESS             0x01
#define     EP0_CMD_POKEX                   0x01    // only for OUT requests
#define     EP0_CMD_PEEKX                   0x02
#define     EP0_CMD_PING0                   0x03
#define     EP0_CMD_PING1                   0x04
#define     EP0_CMD_RESET                   0xfe

#define     DEBUG_CMD_STRING    0xf0
#define     DEBUG_CMD_HEX       0xf1
#define     DEBUG_CMD_HEX16     0xf2
#define     DEBUG_CMD_HEX32     0xf3
#define     DEBUG_CMD_INT       0xf4

#define EP_INBUF_WRITTEN        1
#define EP_OUTBUF_WRITTEN       2


// usb_data bits
#define USBD_CIF_SUSPEND        (uint16_t)0x1
#define USBD_CIF_RESUME         (uint16_t)0x2
#define USBD_CIF_RESET          (uint16_t)0x4
#define USBD_CIF_SOFIF          (uint16_t)0x8
#define USBD_IIF_EP0IF          (uint16_t)0x10
#define USBD_IIF_INEP1IF        (uint16_t)0x20
#define USBD_IIF_INEP2IF        (uint16_t)0x40
#define USBD_IIF_INEP3IF        (uint16_t)0x80
#define USBD_IIF_INEP4IF        (uint16_t)0x100
#define USBD_IIF_INEP5IF        (uint16_t)0x200
#define USBD_OIF_OUTEP1IF       (uint16_t)0x400
#define USBD_OIF_OUTEP2IF       (uint16_t)0x800
#define USBD_OIF_OUTEP3IF       (uint16_t)0x1000
#define USBD_OIF_OUTEP4IF       (uint16_t)0x2000
#define USBD_OIF_OUTEP5IF       (uint16_t)0x4000

#define TXDATA_MAX_WAIT         100

//TODO, port following defines to new cc1111.h
#define  EP_STATE_IDLE      0
#define  EP_STATE_TX        1
#define  EP_STATE_RX        2
#define  EP_STATE_STALL     3

#define USB_STATE_UNCONFIGURED 0
#define USB_STATE_IDLE      1
#define USB_STATE_SUSPEND   2
#define USB_STATE_RESUME    3
#define USB_STATE_RESET     4
#define USB_STATE_WAIT_ADDR 5
#define USB_STATE_BLINK     0xff

// Request Types (bmRequestType)
#define USB_BM_REQTYPE_TGTMASK          0x1f
#define USB_BM_REQTYPE_TGT_DEV          0x00
#define USB_BM_REQTYPE_TGT_INTF         0x01
#define USB_BM_REQTYPE_TGT_EP           0x02
#define USB_BM_REQTYPE_TYPEMASK         0x60
#define USB_BM_REQTYPE_TYPE_STD         0x00
#define USB_BM_REQTYPE_TYPE_CLASS       0x20
#define USB_BM_REQTYPE_TYPE_VENDOR      0x40
#define USB_BM_REQTYPE_TYPE_RESERVED    0x60
#define USB_BM_REQTYPE_DIRMASK          0x80
#define USB_BM_REQTYPE_DIR_OUT          0x00
#define USB_BM_REQTYPE_DIR_IN           0x80
// Standard Requests (bRequest)
#define USB_GET_STATUS                  0x00
#define USB_CLEAR_FEATURE               0x01
#define USB_SET_FEATURE                 0x03
#define USB_SET_ADDRESS                 0x05
#define USB_GET_DESCRIPTOR              0x06
#define USB_SET_DESCRIPTOR              0x07
#define USB_GET_CONFIGURATION           0x08
#define USB_SET_CONFIGURATION           0x09
#define USB_GET_INTERFACE               0x0a
#define USB_SET_INTERFACE               0x11
#define USB_SYNCH_FRAME                 0x12

// Descriptor Types
#define USB_DESC_DEVICE                 0x01
#define USB_DESC_CONFIG                 0x02
#define USB_DESC_STRING                 0x03
#define USB_DESC_INTERFACE              0x04
#define USB_DESC_ENDPOINT               0x05

// USB activities
#define USB_ENABLE_PIN              P1_0
//#define USB_ENABLE_PIN              P1_1
#define NOP()                       __asm; nop; __endasm;
#define USB_DISABLE()               SLEEP &= ~SLEEP_USB_EN;
#define USB_ENABLE()                SLEEP |= SLEEP_USB_EN;
#define USB_RESET()                 USB_DISABLE(); NOP(); USB_ENABLE();
#define USB_INT_ENABLE()            IEN2|= 0x02;
#define USB_INT_DISABLE()           IEN2&= ~0x02;
#define USB_INT_CLEAR()             P2IFG= 0; P2IF= 0;

#define USB_PULLUP_ENABLE()         USB_ENABLE_PIN = 1;
#define USB_PULLUP_DISABLE()        USB_ENABLE_PIN = 0;

#define USB_RESUME_INT_ENABLE()     P0IE= 1
#define USB_RESUME_INT_DISABLE()    P0IE= 0
#define USB_RESUME_INT_CLEAR()      P0IFG= 0; P0IF= 0
#define PM1()                       SLEEP |= 1

SFRX(USBF0,     0xDE20);        // Endpoint 0 FIFO
SFRX(USBF1,     0xDE22);        // Endpoint 1 FIFO
SFRX(USBF2,     0xDE24);        // Endpoint 2 FIFO
SFRX(USBF3,     0xDE26);        // Endpoint 3 FIFO
SFRX(USBF4,     0xDE28);        // Endpoint 4 FIFO
SFRX(USBF5,     0xDE2A);        // Endpoint 5 FIFO

// Power/Control Register
#define USBPOW_SUSPEND_EN       0x01    //rw
#define USBPOW_SUSPEND          0x02    //r
#define USBPOW_RESUME           0x04    //rw
#define USBPOW_RST              0x08    //r
#define USBPOW_ISO_WAIT_SOF     0x80    //rw

#define PICTL_P0ICON                      0x01
#define PICTL_P0IENH                      0x10

typedef struct {
    uint8_t   usbstatus;
    uint16_t  event;
    uint8_t   config;
} USB_STATE;

typedef struct {
    uint8_t*  INbuf;
    uint16_t  INbytesleft;
    uint8_t*  OUTbuf;
    uint16_t  OUTlen;
    uint16_t  BUFmaxlen;
    volatile uint8_t   flags;
    uint8_t   epstatus;
    //__xdata uint8_t*  reg;
    //void*   OUTDONE_handle;                                     // this is a function pointer which is called when the OUT transfer is done.  i may destroy this.
} USB_EP_IO_BUF;

typedef struct USB_Device_Desc_Type {
    uint8_t  bLength;             
    uint8_t  bDescriptorType;     
    uint16_t bcdUSB;                             // cc1111 supports USB v2.0
    uint8_t  bDeviceClass;                       // 0 (each interface defines), 0xff (vendor-specified class code), or a valid class code
    uint8_t  bDeviceSubClass;                    // assigned by USB org
    uint8_t  bDeviceProtocol;                    // assigned by USB org;
    uint8_t  MaxPacketSize;                      // for EP0, 8,16,32,64;
    uint16_t idVendor;                           // assigned by USB org
    uint16_t idProduct;                          // assigned by vendor
    uint16_t bcdDevice;                          // device release number
    uint8_t  iManufacturer;                      // index of the mfg string descriptor
    uint8_t  iProduct;                           // index of the product string descriptor
    uint8_t  iSerialNumber;                      // index of the serial number string descriptor
    uint8_t  bNumConfigurations;                 // number of possible configs...  i wonder if the host obeys this?
} USB_Device_Desc;


typedef struct USB_Config_Desc_Type {
    uint8_t  bLength;             
    uint8_t  bDescriptorType;     
    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;      
    uint8_t  bConfigurationValue; 
    uint8_t  iConfiguration;                     // index of String Descriptor describing this configuration
    uint8_t  bmAttributes;        
    uint8_t  bMaxPower;                          // 2mA increments, 0xfa; 
} USB_Config_Desc;


typedef struct USB_Interface_Desc_Type {
    uint8_t  bLength;             
    uint8_t  bDescriptorType;     
    uint8_t  bInterfaceNumber;
    uint8_t  bAlternateSetting;
    uint8_t  bNumEndpoints;       
    uint8_t  bInterfaceClass;     
    uint8_t  bInterfaceSubClass;  
    uint8_t  bInterfaceProtocol;  
    uint8_t  iInterface;          
} USB_Interface_Desc;


typedef struct USB_Endpoint_Desc_Type {
    uint8_t  bLength;             
    uint8_t  bDescriptorType;     
    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;                       // 0-1 Xfer Type (0;        Isoc, 2;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;                          // Update interval in Frames (for isochronous, ignored for Bulk and Control)
} USB_Endpoint_Desc;


typedef struct USB_LANGID_Desc_Type {
    uint8_t  bLength;
    uint8_t  bDescriptorType;     
    uint16_t wLANGID0;                           // wLANGID[0]  0x0409; 
    uint16_t wLANGID1;                           // wLANGID[1]  0x0c09; 
    uint16_t wLANGID2;                           // wLANGID[1]  0x0407; 
} USB_LANGID_Desc;


typedef struct USB_String_Desc_Type {
    uint8_t   bLength;
    uint8_t   bDescriptorType;     
    uint16_t* bString;
} USB_String_Desc;


typedef struct USB_Request_Type {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} USB_Setup_Header;


// extern global variables
extern USB_STATE usb_data;
extern __xdata uint8_t  usb_ep0_OUTbuf[EP0_MAX_PACKET_SIZE];                  // these get pointed to by the above structure
extern __xdata uint8_t  usb_ep5_OUTbuf[EP5OUT_MAX_PACKET_SIZE];               // these get pointed to by the above structure
extern __xdata USB_EP_IO_BUF     ep0iobuf;
extern __xdata USB_EP_IO_BUF     ep5iobuf;
extern __xdata uint8_t appstatus;

// provided by cc1111usb.c
void usbIntHandler(void) __interrupt P2INT_VECTOR;
void p0IntHandler(void) __interrupt P0INT_VECTOR;
void clock_init(void);
void txdataold(uint8_t app, uint8_t cmd, uint16_t len, uint8_t* dataptr);
void txdata(uint8_t app, uint8_t cmd, uint16_t len, __xdata uint8_t* dataptr);
int setup_send_ep0(uint8_t* payload, uint16_t length);
int setup_sendx_ep0(__xdata uint8_t* payload, uint16_t length);
uint16_t usb_recv_ep0OUT();

uint16_t usb_recv_epOUT(uint8_t epnum, USB_EP_IO_BUF* epiobuf);
void initUSB(void);
void usb_up(void);
void usb_down(void);
void waitForUSBsetup();
// export as this *must* be in main loop.
void usbProcessEvents(void);

// provided by user application
void appHandleEP0OUTdone(void);
int appHandleEP0(USB_Setup_Header* pReq);
int appHandleEP5();

void USBDESCBEGIN(void);
#endif
