/*
 * flymon_sender.ino
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
 * 
 * https://github.com/ggilestro/ardufly
 * flymon sender v1.5
 *
 * This is an ambient temperature, humidity, light sensor
 * Will transmit data wirelessely using the RF12 capabilities of a jeenode
 * To upload, use Arduino IDE and select Duemilanove (jeenode v5) 
 * 
 * REMEMBER: define the node_ID to be unique in your network
 * keeping in mind that 1 is used for the receiver
 * 
 *
 * 
 *
 */

#include <JeeLib.h>
#include <avr/sleep.h>
#include <PortsSHT11.h>

// CHANGE ONLY THIS
#define NODE_ID 3     //define the node ID - Must be unique


//NOTHING TO CHANGE HERE
#define NODE_GRP  5   // wireless net group to use for sending blips
#define SEND_MODE 2   // set to 3 if fuses are e=06/h=DE/l=CE, else set to 2
#define INTERVAL 60   // delay interval for sending data, in seconds

#define DEBUG 0      //run in debug mode
#define VERSION 1.5

SHT11 th_sensor (1);        // SHT on port 1
Port ana (2);               // analog readings on port 2

struct {
    byte id : 7;  // the NODE_ID
    long event; // a growing number counting the events
    float temp; // the temperature measured on the SHT sensor
    float humi; // the humidity measured on the SHT sensor
    byte light; // light measured on the analog sensor
    byte temp_out; // the temperature measured on the TMP36 sensor (optional)
    byte lobat :1;  // supply voltage dropped under 3.1V: 0..1
} outData;

// for low-noise/-power ADC readouts, we'll use ADC completion interrupts
volatile bool adcDone;
ISR(ADC_vect) { adcDone = true; }

// this must be added since we're using the watchdog for low-power waiting
ISR(WDT_vect) {Sleepy::watchdogEvent();}

// spend a little time in power down mode while the SHT11 does a measurement
static void shtDelay () {
    Sleepy::loseSomeTime(32); // must wait at least 20 ms
}

static void serialFlush () {
    #if ARDUINO >= 100
        Serial.flush();
    #endif
    delay(2); // make sure tx buf is empty before going back to sleep
}

static byte myNodeID;       // node ID used for this unit
#define ACK_TIME        10  // number of milliseconds to wait for an ack

// wait a few milliseconds for proper ACK to me, return true if indeed received
static byte waitForAck() {
    MilliTimer ackTimer;
    while (!ackTimer.poll(ACK_TIME)) {
        if (rf12_recvDone() && rf12_crc == 0 &&
                // see http://talk.jeelabs.net/topic/811#post-4712
                rf12_hdr == (RF12_HDR_DST | RF12_HDR_CTL | myNodeID))
            return 1;
        set_sleep_mode(SLEEP_MODE_IDLE);
        sleep_mode();
    }
    return 0;
}

void doMeasure() {
    outData.event++;
    outData.lobat = rf12_lowbat();

    float h, t, d;
    th_sensor.measure(SHT11::HUMI, shtDelay);        
    th_sensor.measure(SHT11::TEMP, shtDelay);
    th_sensor.calculate(h, t);
    d = th_sensor.dewpoint(h, t);
    outData.temp = t;
    outData.humi = h;

    outData.light = ana.anaRead() / 5;
    //outData.temp_out = 0;

    #if DEBUG
        Serial.print("Node id: ");
        Serial.println(outData.id);
        Serial.print("event: ");
        Serial.println(outData.event);
        Serial.print("temp: ");
        Serial.println(outData.temp);
        Serial.print("humi: ");
        Serial.println(outData.humi);
        Serial.print("light: ");
        Serial.println(outData.light);
        Serial.print("temp2: ");
        Serial.println(outData.temp_out);
        Serial.print("low battery: ");
        Serial.println(outData.lobat);
        serialFlush();
    #endif
  
}

void setup() {
    #if DEBUG
      Serial.begin(57600);
      Serial.println( "Transmission started.");
      serialFlush();
    #endif

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
    //rf12_control(0xC040); // set low-battery level to 2.2V i.s.o. 3.1V
    rf12_sleep(RF12_SLEEP);

    outData.event = 0;
    outData.id = NODE_ID;
}


static byte sendPayload () {

  rf12_sleep(RF12_WAKEUP);
  rf12_sendNow(RF12_HDR_ACK, &outData, sizeof outData);
  rf12_sendWait(SEND_MODE);
  byte acked = waitForAck();
  rf12_sleep(RF12_SLEEP);
}

void loop() {
    
  doMeasure();
  sendPayload();
  Sleepy::loseSomeTime(INTERVAL*1000);
  
}    
          
  
