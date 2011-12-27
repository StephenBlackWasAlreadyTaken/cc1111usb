#include "cc1111usbdebug.h"
#include "cc1111usb.h"

/*************************************************************************************************
 * debug stuff.  slows executions.                                                               *
 ************************************************************************************************/
/* blinks the EP0 SETUP packet in binary on the LED */
void debugEP0Req(uint8_t *pReq)
{
    (void) pReq;
    /*
    //uint8_t  loop;

    for (loop = sizeof(USB_Setup_Header);loop>0; loop--)
    {
        blink_binary_baby_lsb(*(pReq), 8);
        pReq++;
    }*/

}


/* sends a debug message up to the python __code to be spit out on stderr */
void debug(__code uint8_t* text)
{
    uint16_t len = 0;
    __code uint8_t* ptr = text;
    while (*ptr++ != 0)
        len ++;
    txdata(0xfe, 0xf0, len, (__xdata uint8_t*)text);
}

void debughex(__xdata uint8_t num)
{
    txdata(0xfe, DEBUG_CMD_HEX, 1, (__xdata uint8_t*)&num);
}

void debughex16(__xdata uint16_t num)
{
    txdata(0xfe, DEBUG_CMD_HEX16, 2, (__xdata uint8_t*)&num);
}

void debughex32(__xdata uint32_t num)
{
    txdata(0xfe, DEBUG_CMD_HEX32, 4, (__xdata uint8_t*)&num);
}
 

