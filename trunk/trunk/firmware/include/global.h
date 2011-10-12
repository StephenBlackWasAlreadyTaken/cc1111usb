#include "cc1111.h"

#ifndef GLOBAL_H
#define GLOBAL_H

/* board-specific defines */
#ifdef IMMEDONGLE
    // CC1110 IMME pink dongle - 26mhz
    #define LED_RED   P2_3
    #define LED_GREEN P2_4
    #define SLEEPTIMER  1100
    
#elif defined DONSDONGLES
    // CC1111 USB Dongle with breakout debugging pins (EMK?) - 24mhz
    #define LED_RED   P1_1
    #define LED_GREEN P1_1
    #define SLEEPTIMER  1200
    #define CC1111EM_BUTTON P1_2

#else
    // CC1111 USB Chronos watch dongle - 24mhz
    #define LED_RED   P1_0
    #define LED_GREEN P1_0
    #define SLEEPTIMER  1200
#endif

#define LED     LED_GREEN


/* non-BLOCKINGBLINK details (ie.  implementing nonblocking blinking */
#define     MAX_BLINK_QUEUE  50

typedef struct {
    u16 index;
    u16 endindex;
    u16 queue[MAX_BLINK_QUEUE];
} BLINK_STATE;

/* function declarations */
void sleepMillis(int ms);
void sleepMicros(int us);
void blink(u16 on_cycles, u16 off_cycles);
void blink_binary_baby_lsb(u16 num, char bits);
#endif
