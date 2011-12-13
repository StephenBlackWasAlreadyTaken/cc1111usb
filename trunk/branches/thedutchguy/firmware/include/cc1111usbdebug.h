#include "global.h"

#ifndef CC1111USBDEBUG_H
#define CC1111USBDEBUG_H

void debugEP0Req(uint8_t *pReq);
void debug(code uint8_t* text);
void debughex(xdata uint8_t num);
void debughex16(xdata uint16_t num);
void debughex32(xdata uint32_t num);

#endif

