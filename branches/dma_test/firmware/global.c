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

/*int reverse(u8* dest, u8* src, int len)
{
    int loop, tmp;
    while (len)
    {
        tmp = *src;
        for (loop^=loop;loop<8;loop++)
        {
            *dest <<= 1; 
            *dest |= (tmp&1);
            tmp >>= 1;
        }
        dest ++;
        src ++;
        len --;
    }
}
*/
/*void blink(u16 on_cycles, u16 off_cycles)                    // haxed for memory usage... made define instead
{
    LED= !LED;
    //LED=1;
    sleepMillis(on_cycles);
    LED= !LED;
    //LED=0;
    sleepMillis(off_cycles);
}*/

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
