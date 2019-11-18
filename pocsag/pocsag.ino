// sendpocsag

// receive POCSAG-message from serial port
// transmit message on 433.900 Mhz using si4432-based "RF22" ISM modules

// This is the "non-ham" version of the application. All references
// to ham call-signs and the CW beacon has been removed

// this program uses the arduino RadioHead library
// http://www.airspayce.com/mikem/arduino/RadioHead/



// (c) 2014 Kristoff Bonne (ON1ARF)
// (c) 2019 Micha LaQua <micha.laqua@gmail.com>

/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This application is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <SPI.h>
#include <RH_RF22.h>

// pocsag message generation library
#include <Pocsag.h>


// Singleton instance of the radio
RH_RF22 rf22;

Pocsag pocsag;

void setup() 
{
  
  Serial.begin(9600);
  if (rf22.init()) {
    Serial.println("RF22 init OK");
  } else {
    Serial.println("RF22 init FAILED");
  }; // end if

  // Set frequency. AfcPullinRange set to 50 Khz (unused, we are not receiving)
  rf22.setFrequency(433.9000, 0.050); // subtract 30 Khz for crystal correction
                                          // this correction-value will differ per device.
                                          // to be determined experimental
  rf22.setModemConfig(RH_RF22::FSK_Rb_1200Fd4_5);
  rf22.setTxPower(RH_RF22_TXPOW_8DBM);  // 6 mW TX power (EU ISM legislation)

  Serial.println("");
  Serial.println("POCSAG text-message tool v0.2 (non-ham):");
  Serial.println("========================================");
  Serial.println("P <address> <sourceid> <repeat> <message>");
  Serial.println("F <freqkhz>    [433050-466800 and 863000-870000]  (default: 433900)");
  Serial.println("D <datarate>    [0 - POCSAG512, 1 - POCSAG1200, 2 - POCSAG2400]  (default: POCSAG1200)");
  Serial.println("X <txpower>    [1-8]  (default: 4)");
  Serial.println("");
}

void loop() {
//vars
int rc;
int state; // state machine for CLI input

// bell = ascii 0x07
char bell=0x07;

// data
long int address;
int sourceid;
int repeat;
char textmsg[42]; // to store text message;
int msgsize;
int freqsize;
int freq1; // freq MHZ part (3 digits)
int freq2; // freq KHZ part (3 digits)
int datarate;
int txpower;

Serial.print("> ");

// init var
state=0;
address=0;
sourceid=0;
msgsize=0;
freqsize=0;
freq1=0; freq2=0;
datarate=-1;
txpower=-1;

while (state >= 0) {
char c;
  // loop until we get a character from serial input
  while (!Serial.available()) {
  }; // end while

  c=Serial.read();


  // break out on ESC and CTRL-C
  if ((c == 0x1b) || (c == 0x03)) {
    state=-999;
    Serial.println("");
    break;
  }; // end if


  // state machine
  if (state == 0) {
    // state 0: wait for command
    // P = PAGE
    // F = FREQUENCY
    // D = DATARATE ID
    // X = TX POWER
        
    if ((c == 'p') || (c == 'P')) {
      // ok, got our "p" -> go to state 1
      state=1;
      
      // echo back char
      Serial.write(c);
    } else if ((c == 'f') || (c == 'F')) {
      // ok, got our "f" -> go to state 10
      state=10;

      // echo back char
      Serial.write(c);
    } else if ((c == 'd') || (c == 'D')) {
      // ok, got our "d" -> go to state 20
      state=20;
      
      // echo back char
      Serial.write(c);
    } else if ((c == 'x') || (c == 'X')) {
      // ok, got our "d" -> go to state 30
      state=30;

      // echo back char
      Serial.write(c);
    } else {
      // error: echo "bell"
      Serial.write(bell);
    }; // end else - if

    // get next char
    continue;
  }; // end state 0

  // state 1: space (" ") or first digit of address ("0" to "9")
  if (state == 1) {
    if (c == ' ') {
      // space -> go to state 2 and get next char
      state=2;

      // echo back char
      Serial.write(c);

      // get next char
      continue;
    } else if ((c >= '0') && (c <= '9')) {
      // digit -> first digit of address. Go to state 2 and process
      state=2;

      // continue to state 2 without fetching next char
    } else {
      // error: echo "bell"
      Serial.write(bell);

      // get next char
      continue;
    }; // end else - if
  };  // end state 1

  // state 2: address ("0" to "9")
  if (state == 2) {
    if ((c >= '0') && (c <= '9')) {
      long int newaddress;

      newaddress = address * 10 + (c - '0');

      if (newaddress <= 0x1FFFFF) {
        // valid address
        address=newaddress;

        Serial.write(c);
      } else {
        // address to high. Send "beep"
        Serial.write(bell);
      }; // end else - if

    } else if (c == ' ') {
      // received space, go to next field (source id)
      Serial.write(c);
      state=3;
    } else {
      // error: echo "bell"
      Serial.write(bell);
    }; // end else - elsif - if

    // get next char
    continue;
  }; // end state 2

  // state 3: source id: one single digit from 0 to 3
  if (state == 3) {
    if ((c >= '0') && (c <= '3')) {
      sourceid= c - '0';
      Serial.write(c);

      state=4;
    } else {
      // invalid: sound bell
      Serial.write(bell);
    }; // end if

    // get next char
    continue;
  }; // end state 3


  // state 4: space between source and repeat
  if (state == 4) {
    if (c == ' ') {
      Serial.write(c);

      state=6; // go from state 4 to state 6
               // (No state 5, callsign removed)
    } else {
      // invalid: sound bell
      Serial.write(bell);
    }; // end if

    // get next char
    continue;
  }; // end state 4


  // state 5: callsign: REMOVED in non-ham version
        
  // state 6: repeat: 1-digit value between 0 and 9
  if (state == 6) {
    if ((c >= '0') && (c <= '9')) {
      Serial.write(c);
      repeat=c - '0';

      // move to next state
      state=7;
    } else {
      Serial.write(bell);
    }; // end if

    // get next char
    continue;
  }; // end state 6


  // state 7: space between repeat and message
  if (state == 7) {
    if (c == ' ') {
      Serial.write(c);

      // move to next state
      state=8;
    } else {
      // invalid char
      Serial.write(bell);
    }; // end else - if

    // get next char
    continue;
  }; // end state 7


  // state 8: message, up to 40 chars, terminate with cr (0x0d) or lf (0x0a)
  if (state == 8) {
    // accepted is everything between space (ascii 0x20) and ~ (ascii 0x7e)
    if ((c >= 0x20) && (c <= 0x7e)) {
      // accept up to 40 chars
      if (msgsize < 40) {
        Serial.write(c);

        textmsg[msgsize]=c;
        msgsize++;
      } else {
        // to long
        Serial.write(bell);
      }; // end else - if
                        
    } else if ((c == 0x0a) || (c == 0x0d)) {
      // done
   
      Serial.println("");
                        
      // add terminating NULL
      textmsg[msgsize]=0x00;

      // break out of loop
      state=-1;
      break;

    } else {
      // invalid char
      Serial.write(bell);
    }; // end else - elsif - if

    // get next char
    continue;
  }; // end state 8;




  // PART 2: frequency

  // state 10: space (" ") or first digit of address ("0" to "9")
  if (state == 10) {
    if (c == ' ') {
      // space -> go to state 11 and get next char
      state=11;

      // echo back char
      Serial.write(c);

      // get next char
      continue;
    } else if ((c >= '0') && (c <= '9')) {
      // digit -> first digit of address. Go to state 2 and process
      state=11;

      // init freqsize;
      freqsize=0;

      // continue to state 2 without fetching next char
    } else {
      // error: echo "bell"
      Serial.write(bell);

      // get next char
      continue;
    }; // end else - if
  };  // end state 10


  // state 11: freq. Mhz part: 3 digits needed
  if (state == 11) {
    if ((c >= '0') && (c <= '9')) {
      if (freqsize < 3) {
        freq1 *= 10;
        freq1 += (c - '0');
               
        freqsize++;
        Serial.write(c);
               
        // go to state 13 if 3 digits received
        if (freqsize == 3) {
          freqsize = 0;
          state = 13;
        }; // end if
      } else {
        // too large: error
        Serial.write(bell);
      }; // end else - if
    } else {
      // unknown char: error
      Serial.write(bell);
    }; // end else - if
          
    // get next char
    continue;
          
  }; // end state 11


  // state 13: part 2 of freq. (KHz part). 3 digits needed
  if (state == 13) {
    if ((c >= '0') && (c <= '9')) {
      if (freqsize < 3) {
        freq2 *= 10;
        freq2 += (c - '0');
               
        freqsize++;
        Serial.write(c);
      } else {
        // too large: error
        Serial.write(bell);
      }; // end else - if
            
      // get next char
      continue;
            
    } else if ((c == 0x0a) || (c == 0x0d)) {
      if (freqsize == 3) {
        // 4 digits received, done
        state = -2;
        Serial.println("");

        // break out
        break;                
      } else {
        // not yet 3 digits
        Serial.write(bell);
                
        // get next char;
        continue;
      }; // end else - if
              
    } else {
      // unknown char
      Serial.write(bell);
             
      // get next char
      continue;
    }; // end else - elsif - if

  }; // end state 12;


  // PART 3: data rate

  // state 20: space (" ") or data rate id (0,1,2)
  if (state == 20) {
    if (c == ' ') {
      state=21;
      Serial.write(c);
      continue;
    } else if ((c >= '0') && (c <= '2')) {
      datarate = c;
      state=21;
      Serial.write(c);
    } else {
      Serial.write(bell);
      continue;
    }
  }

  // state 21: data rate id
  if (state == 21) {
    if ((datarate == -1) && (c >= '0') && (c <= '2')) {
      datarate = c;
      Serial.write(c);
    } else if ((c == 0x0a) || (c == 0x0d)) {
      if (datarate >= 0) {
        state = -3;
        Serial.println("");
        break;
      } else {
        Serial.write(bell);
        continue;
      };
    } else {
      Serial.write(bell);
    }
  }


  // PART 4: tx power

  // state 30: space (" ") or tx power num
  if (state == 30) {
    if (c == ' ') {
      state = 31;
      Serial.write(c);
      continue;
    } else if ((c >= '1') && (c <= '8')) {
      txpower = c;
      state = 31;
      Serial.write(c);
    } else {
      Serial.write(bell);
      continue;
    }
  }

  // state 31: tx power num
  if (state == 31) {
    if ((txpower == -1) && (c >= '1') && (c <= '8')) {
      txpower = c;
      Serial.write(c);
    } else if ((c == 0x0a) || (c == 0x0d)) {
      if (txpower >= 0) {
        state = -4;
        Serial.println("");
        break;
      } else {
        Serial.write(bell);
        continue;
      };
    } else {
      Serial.write(bell);
    }
  }

}; // end while


// Function "P": Send PAGE
if (state == -1) {
  Serial.print("address: ");
  Serial.println(address);

  Serial.print("source id: ");
  Serial.println(sourceid);

  Serial.print("repeat: ");
  Serial.println(repeat);

  Serial.print("message: ");
  Serial.println(textmsg);

  // create pocsag message
  // batch2 option = 0 (truncate message)
  // invert option1 (invert)

  rc=pocsag.CreatePocsag(address,sourceid,textmsg,0,1);


  if (!rc) {
    Serial.print("Error in createpocsag! Error: ");
    Serial.println(pocsag.GetError());

    // sleep 5 seconds
    delay(5000);
  } else {

   
    // send at least once + repeat
    for (int l=-1; l < repeat; l++) {
      Serial.println("POCSAG SEND");

      rf22.send((uint8_t *)pocsag.GetMsgPointer(), pocsag.GetSize());
      rf22.waitPacketSent();
      
      delay(200);
    }; // end for

  }; // end else - if; 
}; // end function P (send PAGE)

// function "F": change frequency

if (state == -2) {
  float newfreq;
  newfreq = ((float) freq1) + ((float) freq2) / 1000.0F;

  // ISM band: 433.050 to 466.800 and 863 to 870
  if ( ((newfreq >= 433.050F) && (newfreq <= 466.8F)) ||
      ((newfreq >= 863.0) && (newfreq < 870.0F)) ) {
    Serial.print("switching to new frequency: ");
    Serial.println(newfreq);

    rf22.setFrequency(newfreq, 0.05); // set frequency, AfcPullinRange not used (receive-only)
  } else {
    Serial.print("invalid frequency: ");
    Serial.println(newfreq);
    Serial.println("allowed frequency range is 433.050-466.800 and 863.000-870.000 Mhz");
  }; // end if
}; // end function F (frequency)

// function "D": change data rate
if (state == -3) {
  Serial.print("switching to new data rate: ");
  if (datarate == '0') {
    Serial.println("POCSAG512");
    rf22.setModemConfig(RH_RF22::FSK_Rb_512Fd4_5);  // (POCSAG512)
  } else if (datarate == '1') {
    Serial.println("POCSAG1200");
    rf22.setModemConfig(RH_RF22::FSK_Rb_1200Fd4_5);  // (POCSAG1200)
  } else if (datarate == '2') {
    Serial.println("POCSAG2400");
    rf22.setModemConfig(RH_RF22::FSK_Rb_2400Fd4_5);  // (POCSAG2400)
  }
} // end function D (data rate)

// function "X": change tx power
if (state == -4) {
  Serial.print("setting tx power to: ");
  if (txpower == '1') {
    Serial.println("1 (1 dBm)");
    rf22.setTxPower(RH_RF22_TXPOW_1DBM);
  } else if (txpower == '2') {
    Serial.println("2 (2 dBm)");
    rf22.setTxPower(RH_RF22_TXPOW_2DBM);
  } else if (txpower == '3') {
    Serial.println("3 (5 dBm)");
    rf22.setTxPower(RH_RF22_TXPOW_5DBM);
  } else if (txpower == '4') {
    Serial.println("4 (8 dBm)");
    rf22.setTxPower(RH_RF22_TXPOW_8DBM);
  } else if (txpower == '5') {
    Serial.println("5 (11 dBm)");
    rf22.setTxPower(RH_RF22_TXPOW_11DBM);
  } else if (txpower == '6') {
    Serial.println("6 (14 dBm)");
    rf22.setTxPower(RH_RF22_TXPOW_14DBM);
  } else if (txpower == '7') {
    Serial.println("7 (17 dBm)");
    rf22.setTxPower(RH_RF22_TXPOW_17DBM);
  } else if (txpower == '8') {
    Serial.println("8 (20 dBm)");
    rf22.setTxPower(RH_RF22_TXPOW_20DBM);
  }
} // end function X (tx power)

}; // end main application





