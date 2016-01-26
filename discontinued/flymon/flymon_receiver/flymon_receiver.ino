/*
 * flymoin_receiver.ino
 * 
 * Copyright 2013 Giorgio Gilestro <giorgio@gilest.ro>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * Check https://plot.ly/ for automatic data upload
 * 
 * https://github.com/ggilestro/ardufly
 * flymon receiver
 *
 * This is an ambient temperature, humidity, light sensor
 * Will transmit data wirelessely using the RF12 capabilities of a jeenode
 * 
 * This is based on a jeenode v5
 * To upload, use Arduino IDE and select Arduino Duemilanove 
 *
 * REMEMBER: define the node_ID to be unique in your network
 * keeping in mind that 1 is used for the receiver
 * 
 */
 
#include <JeeLib.h>

#define NODE_ID 1     //define the node ID - Must be unique - 1 is for the receiver
#define NODE_GRP  5   // wireless net group to use for sending blips
#define SEND_MODE 2   // set to 3 if fuses are e=06/h=DE/l=CE, else set to 2

#define VERSION 1.5

struct {
    byte id :7;  // the NODE_ID
    long event; // a growing number counting the events
    float temp; // the temperature measured on the SHT sensor
    float humi; // the humidity measured on the SHT sensor
    byte light; // light measured on the analog sensor
    byte temp_out; // the temperature measured on the TMP36 sensor (optional)
    byte lobat :1;  // supply voltage dropped under 3.1V: 0..1
} inData;


static void serialFlush () {
    #if ARDUINO >= 100
        Serial.flush();
    #endif  
    delay(2); // make sure tx buf is empty before going back to sleep
}

void setup () {
    Serial.begin(57600);
    Serial.print( "FLYMON RECEIVER VERSION ");
    Serial.print (VERSION);
    Serial.println( " - Ready.");
    serialFlush();
//    
//    // get the pre-scaler into a known state
//    cli();
//    CLKPR = bit(CLKPCE);
//    #if defined(__AVR_ATtiny84__)
//      CLKPR = 0; // div 1, i.e. speed up to 8 MHz
//    #else
//      CLKPR = 1; // div 2, i.e. slow down to 8 MHz
//    #endif
//      sei();
//
//    #if defined(__AVR_ATtiny84__)
//        // power up the radio on JMv3
//        bitSet(DDRB, 0);
//        bitClear(PORTB, 0);
//    #endif

    rf12_initialize(NODE_ID, RF12_868MHZ, NODE_GRP);
    // see http://tools.jeelabs.org/rfm12b
//    rf12_control(0xC040); // set low-battery level to 2.2V i.s.o. 3.1V
    rf12_sleep(RF12_SLEEP);

}

static void consumeInData () {
    Serial.print(inData.id);
    Serial.print(",");
    Serial.print(inData.event);
    Serial.print(",");
    Serial.print(inData.temp);
    Serial.print(",");
    Serial.print(inData.humi);
    Serial.print(",");
    Serial.print(inData.light);
    Serial.print(",");
    Serial.print(inData.temp_out);
    Serial.print(",");
    Serial.println(inData.lobat);
    serialFlush();
}

void loop () {
    if (rf12_recvDone() && rf12_crc == 0 && rf12_len == sizeof inData) {
        memcpy(&inData, (byte*) rf12_data, sizeof inData);
        // optional: rf12_recvDone(); // re-enable reception right away
        consumeInData();
    }
}

