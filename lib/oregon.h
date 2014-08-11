// Oregon V2 and Cresta Decoder mofified by Sidey
// Oregon V2 decoder modfied - Olivier Lebrun
// Oregon V2 decoder added - Dominique Pierre
/// @file
/// Generalized decoder framework for 868 MHz and 433 MHz OOK signals.
// 2010-04-11 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

//#define DEBUG           // Compile with Debug informations

#ifndef _OREGON_h
  #define _OREGON_h
  #if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
  #else
    #include "WProgram.h"
  #endif
#endif

#include "sketch.h"

class DecodeOOK {
  protected:
    byte total_bits, bits, flip, state, pos, data[25];

    virtual char decode (word width) = 0;

  public:

    enum { UNKNOWN, T0, T1, T2, T3, OK, DONE };

    DecodeOOK () {
      resetDecoder();
    }

    bool nextPulse (word width) {
      if (state != DONE)

        switch (decode(width)) {
          case -1: resetDecoder(); break;
          case 1:  done(); break;
        }
      return isDone();
    }

    bool isDone () const {
      return state == DONE;
    }

    const byte* getData (byte& count) const {
      count = pos;
      return data;
    }

    void resetDecoder () {
      total_bits = bits = pos = flip = 0;
      state = UNKNOWN;
    }

    // add one bit to the packet data buffer

    virtual void gotBit (char value) {
      total_bits++;
      byte *ptr = data + pos;
      *ptr = (*ptr >> 1) | (value << 7);

      if (++bits >= 8) {
        bits = 0;
        if (++pos >= sizeof data) {
          resetDecoder();
          return;
        }
      }
      state = OK;
    }

    // store a bit using Manchester encoding
    void manchester (char value) {
      flip ^= value; // manchester code, long pulse flips the bit
      gotBit(flip);
    }

    // move bits to the front so that all the bits are aligned to the end
    void alignTail (byte max = 0) {
      // align bits
      if (bits != 0) {
        data[pos] >>= 8 - bits;
        for (byte i = 0; i < pos; ++i)
          data[i] = (data[i] >> bits) | (data[i + 1] << (8 - bits));
        bits = 0;
      }
      // optionally shift bytes down if there are too many of 'em
      if (max > 0 && pos > max) {
        byte n = pos - max;
        pos = max;
        for (byte i = 0; i < pos; ++i)
          data[i] = data[i + n];
      }
    }

    void reverseBits () {
      for (byte i = 0; i < pos; ++i) {
        byte b = data[i];
        for (byte j = 0; j < 8; ++j) {
          data[i] = (data[i] << 1) | (b & 1);
          b >>= 1;
        }
      }
    }

    void reverseNibbles () {
      for (byte i = 0; i < pos; ++i)
        data[i] = (data[i] << 4) | (data[i] >> 4);
    }

    void done () {
      while (bits)
        gotBit(0); // padding
      state = DONE;
    }
};

#ifdef COMP_OSV2
class OregonDecoderV2 : public DecodeOOK {
  public:

    OregonDecoderV2() {}

    // add one bit to the packet data buffer
    virtual void gotBit (char value) {
      if (!(total_bits & 0x01))
      {
        data[pos] = (data[pos] >> 1) | (value ? 0x80 : 00);
      }
      total_bits++;
      pos = total_bits >> 4;
      if (pos >= sizeof data) {
        resetDecoder();
        return;
      }
      state = OK;
    }

    virtual char decode (word width) {
      if (200 <= width && width < 1200) {
        //Serial.print("Dauer="); Serial.println(width);
        //Serial.println(width);
        byte w = width >= 700;

        switch (state) {
          case UNKNOWN:
            if (w != 0) {
              // Long pulse
              ++flip;
            } else if (w == 0 && 24 <= flip) {
              // Short pulse, start bit
              flip = 0;
              state = T0;
            } else {
              // Reset decoder
              return -1;
            }
            break;
          case OK:
            if (w == 0) {
              // Short pulse
              state = T0;
            } else {
              // Long pulse
              manchester(1);
            }
            break;
          case T0:
            if (w == 0) {
              // Second short pulse
              manchester(0);
            } else {
              // Reset decoder
              return -1;
            }
            break;
        }
      } else if (width >= 2500  && pos >= 8) {
        return 1;
      } else {
        return -1;
      }
      return 0;
    }
};


#endif

#ifdef COMP_OSV3
class OregonDecoderV3 : public DecodeOOK {
  public:
    OregonDecoderV3() {}

    // add one bit to the packet data buffer
    virtual void gotBit (char value) {
      data[pos] = (data[pos] >> 1) | (value ? 0x80 : 00);
      total_bits++;
      pos = total_bits >> 3;
      if (pos >= sizeof data) {
        resetDecoder();
        return;
      }
      state = OK;
    }

    virtual char decode (word width) {
      if (200 <= width && width < 1200) {
        byte w = width >= 700;
        switch (state) {
          case UNKNOWN:
            if (w == 0)
              ++flip;
            else if (32 <= flip) {
              flip = 1;
              manchester(1);
            } else
              return -1;
            break;
          case OK:
            if (w == 0)
              state = T0;
            else
              manchester(1);
            break;
          case T0:
            if (w == 0)
              manchester(0);
            else
              return -1;
            break;
        }
      } else {
        return -1;
      }
      return  total_bits == 80 ? 1 : 0;
    }
};
#endif

#ifdef COMP_Cresta
class CrestaDecoder : public DecodeOOK {
  public:
    CrestaDecoder () {}

    const byte* getData (byte& count) const {

      count = pos;
      return data;
    }

    // add one bit to the packet data buffer
virtual void gotBit (char value) {
    total_bits++;
    byte *ptr = data + pos;

    if (++bits < 9) {
        *ptr = (*ptr >> 1) | (value << 7);
    } else {
        if (pos > 0) {
            *ptr = *ptr ^ (*ptr << 1);
        }

        bits = 0;

        if (++pos >= sizeof data) {
            resetDecoder();
            return;
        }
    }

    state = OK;
}


      virtual char decode (word width) {
        if (pos > 0 && data[0] != 0x75) {
          //packets start with 0x75!
          return -1;
        }

        if (200 <= width && width < 1300) {
          byte w = width >= 750;
          switch (state) {
          case UNKNOWN:
          case OK:
            if (w == 0)
              state = T0;
            else
              manchester(1);
            break;
          case T0:
            if (w == 0)
              manchester(0);
            else
              return -1;
            break;
          }

          if (pos > 6) {
            byte len = 3 + ((data[2] >> 1) & 0x1f); //total packet len
            byte csum = 0;
            for (byte x = 1; x < len-1; x++) {
              csum ^= data[x];
            }

            if (len == pos) {
              if (csum == 0) {
                return 1;
              } 
              else {
                return -1;
              }
            }
          } 
        } 
        else
          return -1;

        return 0;
      }
  


};


#endif

#ifdef COMP_KAKU
class KakuDecoder : public DecodeOOK {
  public:
    KakuDecoder () {}

    virtual char decode (word width) {
      if (180 <= width && width < 450 || 950 <= width && width < 1250) {
        byte w = width >= 700;
        switch (state) {
          case UNKNOWN:
          case OK:
            if (w == 0)
              state = T0;
            else
              return -1;
            break;
          case T0:
            if (w)
              state = T1;
            else
              return -1;
            break;
          case T1:
            state += w + 1;
            break;
          case T2:
            if (w)
              gotBit(0);
            else
              return -1;
            break;
          case T3:
            if (w == 0)
              gotBit(1);
            else
              return -1;
            break;
        }
      } else if (width >= 2500 && 8 * pos + bits == 12) {
        for (byte i = 0; i < 4; ++i)
          gotBit(0);
        alignTail(2);
        return 1;
      } else
        return -1;
      return 0;
    }
};

#endif

#ifdef COMP_XRF
class XrfDecoder : public DecodeOOK {
  public:
    XrfDecoder () {}

    // see also http://davehouston.net/rf.htm
    virtual char decode (word width) {
      if (width > 2000 && pos >= 4)
        return 1;
      if (width > 5000)
        return -1;
      if (width > 4000 && state == UNKNOWN)
        state = OK;
      else if (350 <= width && width < 1800) {
        byte w = width >= 720;
        switch (state) {
          case OK:
            if (w == 0)
              state = T0;
            else
              return -1;
            break;
          case T0:
            gotBit(w);
            break;
        }
      } else
        return -1;
      return 0;
    }
};
#endif

#ifdef COMP_HEZ
class HezDecoder : public DecodeOOK {
  public:
    HezDecoder () {}

    // see also http://homeeasyhacking.wikia.com/wiki/Home_Easy_Hacking_Wiki
    virtual char decode (word width) {
      if (200 <= width && width < 1200) {
        byte w = width >= 600;
        gotBit(w);
      } else if (width >= 5000 && pos >= 5 /*&& 8 * pos + bits == 50*/) {
        for (byte i = 0; i < 6; ++i)
          gotBit(0);
        alignTail(7); // keep last 56 bits
        return 1;
      } else
        return -1;
      return 0;
    }
};
#endif
