#include "global.h"

void sleepMillis(int ms) 
{
    int j;
    while (--ms > 0) 
    { 
        for (j=0; j<SLEEPTIMER;j++); // about 1 millisecond
    }
}


void sleepMicros(int us) 
{
    while (--us > 0) ;
}


/* FIXME: not convinced libc hurts us that much
int memcpy(volatile xdata void* dst, volatile xdata void* src, u16 len)
{
    u16 loop;
    for (loop^=loop;loop<len; loop++)
    {
        *(dst++) = *(src++);
    }
    return loop+1;
}

int memset(volatile xdata void* dst, const char ch, u16 len)
{
    u16 loop;
    for (loop^=loop;loop<len; loop++)
    {
        *(ptr++) = 0;
    }
    return loop+1;
}
*/
