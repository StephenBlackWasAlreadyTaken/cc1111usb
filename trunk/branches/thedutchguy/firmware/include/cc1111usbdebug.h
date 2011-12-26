#ifndef CC1111USBDEBUG_H
#define CC1111USBDEBUG_H

#include "global.h"

void debugEP0Req(uint8_t *pReq);
void debug(__code uint8_t* text);
void debughex(__xdata uint8_t num);
void debughex16(__xdata uint16_t num);
void debughex32(__xdata uint32_t num);

#endif

