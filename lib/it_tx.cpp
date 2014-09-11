/*-----------------------------------------------------------------------------------------------
/* Devices with temperatur / humidity functionality => Intertechno TX2/3/4
-----------------------------------------------------------------------------------------------*/

#include "it_tx.h"

extern String cmdstring;
extern volatile bool available;
extern String message;

#define TX_MAX_CHANGES 88
unsigned int timingsTX[TX_MAX_CHANGES+20];      //  TX

// TFA 30.3125 sends 1330 for low, 540 for high with 1000 pauses. 
// humidity is even longer
#define TX_ONE     520
#define TX_ZERO   1300
#define TX_GLITCH  100
#define TX_GLITCH2 200
#define TX_PAUSE  1000
#define TX_MESSAGELENGTH 44

// http://www.f6fbb.org/domo/sensors/tx3_th.php
void IT_TX(unsigned int duration) {

  static unsigned int changeCount;
  static unsigned int fDuration;
  static unsigned int sDuration;

  sDuration = fDuration + duration;
  
  if ((sDuration > TX_PAUSE + TX_ZERO - TX_GLITCH2 && 
       sDuration < TX_PAUSE + TX_ZERO + TX_GLITCH2 ) || 
      (sDuration > TX_PAUSE + TX_ONE  - TX_GLITCH2 && 
       sDuration < TX_PAUSE + TX_ONE  + TX_GLITCH2)) {
    if ((duration > TX_ONE   - TX_GLITCH && duration < TX_ONE   + TX_GLITCH) || 
        (duration > TX_ZERO  - TX_GLITCH && duration < TX_ZERO  + TX_GLITCH) || 
        (duration > TX_PAUSE - TX_GLITCH && duration < TX_PAUSE + TX_GLITCH)) {
      if (changeCount == 0 ) {
        timingsTX[changeCount++] = fDuration;
      }
      timingsTX[changeCount++] = duration;
    }

    if ( changeCount == TX_MAX_CHANGES - 1) {
      receiveProtocolIT_TX(changeCount);
      changeCount = 0;
      fDuration = 0;
    } 
  } else {
    changeCount = 0;
  }

  fDuration = duration;
}

void receiveProtocolIT_TX(unsigned int changeCount) {

  byte i;
  unsigned long code = 0;

#ifdef USE_IT_TX
  message = "TX";
#else
  message = "W00";
#endif

  for (i = 0; i <= 14; i = i + 2)
  {
    if ((timingsTX[i] > TX_ZERO - TX_GLITCH) && (timingsTX[i] < TX_ZERO + TX_GLITCH))    {
      code <<= 1;
    }
    else if ((timingsTX[i] > TX_ONE - TX_GLITCH) && (timingsTX[i] < TX_ONE + TX_GLITCH)) {
      code <<= 1;
      code |= 1;
    }
    else {
      return;
    }
  }

  // Startsequence 0000 1010 = 0xA
  if (code != 10) {
    return;
  }

  message += String(code,HEX);
  code = 0;

  // Sensor type 0000 = Temp / 1110 = Humidity
  for (i = 16; i <= 22; i = i + 2)
  {
    if ((timingsTX[i] > TX_ZERO - TX_GLITCH) && (timingsTX[i] < TX_ZERO + TX_GLITCH))    {
      code <<= 1;
    }
    else if ((timingsTX[i] > TX_ONE - TX_GLITCH) && (timingsTX[i] < TX_ONE + TX_GLITCH)) {
      code <<= 1;
      code |= 1;
    }
    else {
      return;
    }
  }

  message += String(code,HEX);
  code = 0;

  // Sensor adress
  for (i = 24; i <= 38; i = i + 2)
  {
    if ((timingsTX[i] > TX_ZERO - TX_GLITCH) && (timingsTX[i] < TX_ZERO + TX_GLITCH))    {
      code <<= 1;
    }
    else if ((timingsTX[i] > TX_ONE - TX_GLITCH) && (timingsTX[i] < TX_ONE + TX_GLITCH)) {
      code <<= 1;
      code |= 1;
    }
    else {
      return;
    }
  }
  message += String(code,HEX);
  code = 0;

  // Temp or Humidity
  for (i = 40; i <= 62; i = i + 2)
  {
    if ((timingsTX[i] > TX_ZERO - TX_GLITCH) && (timingsTX[i] < TX_ZERO + TX_GLITCH))    {
      code <<= 1;
    }
    else if ((timingsTX[i] > TX_ONE - TX_GLITCH) && (timingsTX[i] < TX_ONE + TX_GLITCH)) {
      code <<= 1;
      code |= 1;
    }
    else {
      return;
    }
  }
  message += String(code,HEX);
  code = 0;

  // Repeated Bytes temp / Humidity
  for (i = 64; i <= 78; i = i + 2)
  {
    if ((timingsTX[i] > TX_ZERO - TX_GLITCH) && (timingsTX[i] < TX_ZERO + TX_GLITCH))    {
      code <<= 1;
    }
    else if ((timingsTX[i] > TX_ONE - TX_GLITCH) && (timingsTX[i] < TX_ONE + TX_GLITCH)) {
      code <<= 1;
      code |= 1;
    }
    else {
      return;
    }
  }
  message += String(code,HEX);
  code = 0;

  // Checksum
  for (i = 80; i <= changeCount; i = i + 2)
  {
    if ((timingsTX[i] > TX_ZERO - TX_GLITCH) && (timingsTX[i] < TX_ZERO + TX_GLITCH))    {
      code <<= 1;
    }
    else if ((timingsTX[i] > TX_ONE - TX_GLITCH) && (timingsTX[i] < TX_ONE + TX_GLITCH)) {
      code <<= 1;
      code |= 1;
    }
    else {
      return;
    }
  }
  message += String(code,HEX);
  
  message.toUpperCase();

  available = true;
  return;
}

