/*-----------------------------------------------------------------------------------------------
/* Somfy RTS window shades (receiver only) --> viegener
-----------------------------------------------------------------------------------------------*/

//#define DEBUG           // Compile with Debug informations

#ifndef _SOMFYRTS_h
  #define _SOMFYRTS_h
  #if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
  #else
    #include "WProgram.h"
  #endif
#endif

#include "sketch.h"
#include "helper.h"

/*
 * SOMFY RTS Receiver 
 */

// Timings and limits
#define LOW_FACTOR 0.7
#define HIGH_FACTOR 1.4

#define TIME_FULL_SYMBOL_LOW   (1280 * LOW_FACTOR)
#define TIME_FULL_SYMBOL_HIGH  (1280 * HIGH_FACTOR)

#define TIME_HALF_SYMBOL_LOW   (640 * LOW_FACTOR)
#define TIME_HALF_SYMBOL_HIGH  (640 * HIGH_FACTOR)

#define TIME_PRESYNC_LOW   (2416 * LOW_FACTOR)
#define TIME_PRESYNC_HIGH  (2416 * HIGH_FACTOR)

#define TIME_SYNC_LOW   (4550 * LOW_FACTOR)
#define TIME_SYNC_HIGH  (4550 * HIGH_FACTOR)

#define MIN_PRESYNC_COUNT 4


 
// interrupt handler
void somfyHandler(unsigned int duration);

// internal helper for interrupt
boolean handleChange( unsigned int duration );


#ifdef DEBUG
// print message human readable on Serial --> for debug
void printMessage();

#endif


