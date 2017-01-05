/*-----------------------------------------------------------------------------------------------
 * Somfy RTS window shades (receiver only) --> viegener
 /----------------------------------------------------------------------------------------------*/

#include "somfyRTS.h"

extern volatile bool available;
extern String message;

/*
 * Receiver
 */
// int for interupt routine relevant variables
volatile boolean int_hasSync = false;

volatile byte int_bitCount;
volatile byte int_presyncCount;

volatile byte int_messageDone[7];

// Interrupt handler
void somfyHandler(unsigned int duration) {
    // calculate last change duration (and store last time)
    if ( int_hasSync ) {
//      Serial.print("Duration : ");
//      Serial.println( duration );
      int_hasSync = handleChange( duration );
/*
      if ( ( int_bitCount % 10 ) == 0 ) {
        Serial.print("bc : ");
        Serial.println( int_bitCount );
      }
*/
    } else {

      if ( ( duration > TIME_PRESYNC_LOW ) && ( duration < TIME_PRESYNC_HIGH ) ) {
        int_presyncCount++;
      } else if ( ( duration > TIME_SYNC_LOW ) && ( duration < TIME_SYNC_HIGH ) && ( int_presyncCount >= MIN_PRESYNC_COUNT ) ) {
        int_hasSync = handleChange( 0 );
      } else {
        int_presyncCount = 0;
      }
    }
}      
      


// helper for decoding bits
// call with duration 0 to prepare for data receive
// message format Y <key-2-hex> <cmd-2-hex> <rollingcode-4-hex> <address-6-hex> 
boolean handleChange( unsigned int duration ) {
  static byte lastBitValue;
  static boolean hasHalfBit;
  static byte int_message[7];
 
  if ( duration == 0 ) {
    // init for receive
    lastBitValue = 0;
    int_bitCount = 0;
    hasHalfBit = false;

    for(int i=0; i<7; ++i) {
      int_message[i] = 0;
    }
  
    // init for next sync
    int_presyncCount = 0;
    
  } else if ( ( ! hasHalfBit ) && ( duration > TIME_FULL_SYMBOL_LOW ) && ( duration < TIME_FULL_SYMBOL_HIGH ) ) {
    lastBitValue = 1 - lastBitValue;
    int_message[ int_bitCount / 8 ] += lastBitValue << ( 7 - (int_bitCount % 8 ) );
    int_bitCount++;
  } else if ( ( duration > TIME_HALF_SYMBOL_LOW ) && ( duration < TIME_HALF_SYMBOL_HIGH ) ) {
    if ( hasHalfBit ) {
      int_message[ int_bitCount / 8 ] += lastBitValue << ( 7 - (int_bitCount % 8 ) );
      int_bitCount++;
    }    
    hasHalfBit = ! hasHalfBit;
  } else {
#ifdef DEBUG
    Serial.print("Receive failed : ");
    Serial.print(int_bitCount);
    Serial.print(":   duration : ");
    Serial.print(duration);
    Serial.println("::");
#endif
    // abort receive due to duration not in tolerated ranges 
    return false;
  }    
    
  if ( int_bitCount == 56) {
#ifdef DEBUG
    Serial.print("Frame received : ");
    Serial.println("::");
#endif
    if ( ! messageAvailable() ) {
      // obfuscate the frame to output (int_messageDone) and calculate checksum
      int_messageDone[0] = int_message[0];
      byte checkSum = 0 ^ int_messageDone[0] ^ (int_messageDone[0] >> 4 );
      for(int i=1; i<7; i++) {
        int_messageDone[i] = int_message[i] ^ int_message[i-1];
        checkSum ^= int_messageDone[i] ^ (int_messageDone[i] >> 4 );
      }

      // set message ready when checksum is correct
      if ( (checkSum & 0x0F) == 0 ) {
//	# message looks like this
//	# Ys_key_ctrl_cks_rollcode_a2a1a0
//	# Ys ad 20 0ae3 a29842
//	# address needs bytes 1 and 3 swapped
//      # lenght is 2+1 2+1 2+1 4+1 6 = 20

        String tmpMessage = "Ys ";
         char tmp[7];
         
         // key
         sprintf( tmp, "%2.2x ", int_messageDone[0] );
         tmpMessage += tmp;
         // command
         sprintf( tmp, "%2.2x ", (int_messageDone[1] & 0xF0) );
         tmpMessage += tmp;
         // rolling code
         sprintf( tmp, "%4.4x ", (int_messageDone[2] << 8) + int_messageDone[3] );
         tmpMessage += tmp;
         // address
         sprintf( tmp, "%2.2x%2.2x%2.2x", int_messageDone[4],int_messageDone[5],int_messageDone[6] );
         tmpMessage += tmp;
         message = tmpMessage;
         available = true;
         
#ifdef DEBUG

	printMessage();
      } else {
        Serial.print("CheckSum incorrect : ");
        Serial.println( (checkSum & 0x0F), HEX);

#endif  
      }
    }
    
    // message complete --> start over 
    return false;
  }

  return true;
}

#ifdef DEBUG

void printMessage() {

  Serial.print("Key : "); 
  Serial.print(int_messageDone[0], HEX);

  // Adresse
  Serial.print(": Adr: "); 
  Serial.print(int_messageDone[4], HEX);
  Serial.print(":"); 
  Serial.print(int_messageDone[5], HEX);
  Serial.print(":"); 
  Serial.print(int_messageDone[6], HEX);

  // Different commands (lower nibble is checksum)
  Serial.print(":    Cmd "); 
  switch(int_messageDone[1] & 0xF0) {
    case 0x10: Serial.print("Stop"); break;
    case 0x20: Serial.print("Up  "); break;
    case 0x40: Serial.print("Down"); break;
    case 0x80: Serial.print("Prog"); break;
    default: Serial.print("?? ");Serial.print(int_messageDone[1] >> 4, HEX); break;
  }
  
  // Rolling code
  Serial.print("    RollCode: "); 
  unsigned long rolling_code = (int_messageDone[2] << 8) + int_messageDone[3];
  Serial.print(rolling_code);
  
  Serial.println("  "); 
}

#endif  

