#ifndef CC1111USB
#define CC1111USB

#include "cc1111.h"
#include "global.h"

#define     EP0_MAX_PACKET_SIZE     64
#define     EP5OUT_MAX_PACKET_SIZE  255
#define     EP5IN_MAX_PACKET_SIZE   255
//   #define     EP5_MAX_PACKET_SIZE     255
        // note: descriptor needs to be adjusted to match EP5_MAX_PACKET_SIZE

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

typedef struct USB_Device_Desc_Type {
    uint8  bLength;             
    uint8  bDescriptorType;     
    uint16 bcdUSB;                             // cc1111 supports USB v2.0
    uint8  bDeviceClass;                       // 0 (each interface defines), 0xff (vendor-specified class code), or a valid class code
    uint8  bDeviceSubClass;                    // assigned by USB org
    uint8  bDeviceProtocol;                    // assigned by USB org;
    uint8  MaxPacketSize;                      // for EP0, 8,16,32,64;
    uint16 idVendor;                           // assigned by USB org
    uint16 idProduct;                          // assigned by vendor
    uint16 bcdDevice;                          // device release number
    uint8  iManufacturer;                      // index of the mfg string descriptor
    uint8  iProduct;                           // index of the product string descriptor
    uint8  iSerialNumber;                      // index of the serial number string descriptor
    uint8  bNumConfigurations;                 // number of possible configs...  i wonder if the host obeys this?
} USB_Device_Desc;


typedef struct USB_Config_Desc_Type {
    uint8  bLength;             
    uint8  bDescriptorType;     
    uint16 wTotalLength;
    uint8  bNumInterfaces;      
    uint8  bConfigurationValue; 
    uint8  iConfiguration;                     // index of String Descriptor describing this configuration
    uint8  bmAttributes;        
    uint8  bMaxPower;                          // 2mA increments, 0xfa; 
} USB_Config_Desc;


typedef struct USB_Interface_Desc_Type {
    uint8  bLength;             
    uint8  bDescriptorType;     
    uint8  bInterfaceNumber;
    uint8  bAlternateSetting;
    uint8  bNumEndpoints;       
    uint8  bInterfaceClass;     
    uint8  bInterfaceSubClass;  
    uint8  bInterfaceProtocol;  
    uint8  iInterface;          
} USB_Interface_Desc;


typedef struct USB_Endpoint_Desc_Type {
    uint8  bLength;             
    uint8  bDescriptorType;     
    uint8  bEndpointAddress;
    uint8  bmAttributes;                       // 0-1 Xfer Type (0;        Isoc, 2;
    uint16 wMaxPacketSize;
    uint8  bInterval;                          // Update interval in Frames (for isochronous, ignored for Bulk and Control)
} USB_Endpoint_Desc;


typedef struct USB_LANGID_Desc_Type {
    uint8  bLength;
    uint8  bDescriptorType;     
    uint16 wLANGID0;                           // wLANGID[0]  0x0409; 
    uint16 wLANGID1;                           // wLANGID[1]  0x0c09; 
    uint16 wLANGID2;                           // wLANGID[1]  0x0407; 
} USB_LANGID_Desc;


typedef struct USB_String_Desc_Type {
    uint8   bLength;
    uint8   bDescriptorType;     
    uint16* bString;
} USB_String_Desc;


typedef struct USB_Request_Type {
    uint8  bmRequestType;
    uint8  bRequest;
    uint16 wValue;
    uint16 wIndex;
    uint16 wLength;
} USB_Setup_Header;


// extern global variables
extern USB_STATE usb_data;
extern xdata u8  usb_ep0_OUTbuf[EP0_MAX_PACKET_SIZE];                  // these get pointed to by the above structure
extern xdata u8  usb_ep5_OUTbuf[EP5OUT_MAX_PACKET_SIZE];               // these get pointed to by the above structure
extern xdata USB_EP_IO_BUF     ep0iobuf;
extern xdata USB_EP_IO_BUF     ep5iobuf;
extern xdata u8 appstatus;

extern xdata u8   ep0req;
extern xdata u16  ep0len;
extern xdata u16  ep0value;

// provided by cc1111usb.c
void usbIntHandler(void) interrupt P2INT_VECTOR;
void p0IntHandler(void) interrupt P0INT_VECTOR;
void clock_init(void);
void txdataold(u8 app, u8 cmd, u16 len, u8* dataptr);
void txdata(u8 app, u8 cmd, u16 len, xdata u8* dataptr);
int setup_send_ep0(u8* payload, u16 length);
int setup_sendx_ep0(xdata u8* payload, u16 length);
u16 usb_recv_ep0OUT();

u16 usb_recv_epOUT(u8 epnum, USB_EP_IO_BUF* epiobuf);
void initUSB(void);
void usb_up(void);
void usb_down(void);
void waitForUSBsetup();
// export as this *must* be in main loop.
void usbProcessEvents(void);


// provided by user application
void appHandleEP0OUTdone(void);
void appHandleEP0OUT(void);
int appHandleEP0(USB_Setup_Header* pReq);
int appHandleEP5();



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

#define TXDATA_MAX_WAIT         30


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
               ;//.DB 0xf4, 0x01              ; wMaxPacketSize
               .DB 0xff, 0x00              ; wMaxPacketSize
               .DB 0x01                    ; bInterval
0005$:  ; Endpoint descriptor (EP5 OUT)
               .DB 0006$ - 0005$           ; bLength
               .DB USB_DESC_ENDPOINT       ; bDescriptorType
               .DB 0x05                    ; bEndpointAddress
               .DB 0x02                    ; bmAttributes
               .DB 0xff, 0x00              ; wMaxPacketSize
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
               .DB "n", 0
               .DB "i", 0
               .DB "c", 0
               ;//.DB "k", 0
               ;//.DB "a", 0
               ;//.DB "s", 0
               ;//.DB "s", 0
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

#endif
