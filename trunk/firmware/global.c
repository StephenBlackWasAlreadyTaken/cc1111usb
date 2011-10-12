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
void blink(u16 on_cycles, u16 off_cycles)
{
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
