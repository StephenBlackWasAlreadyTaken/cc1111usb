#ifndef GLOBAL_H
#define GLOBAL_H

#define RECEIVE_TEST

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

#define blink( on_cycles, off_cycles)  {LED=1; sleepMillis(on_cycles); LED=0; sleepMillis(off_cycles);}
#define SLEEPTIMER  1200

void sleepMillis(int ms);
void sleepMicros(int us);

#endif
