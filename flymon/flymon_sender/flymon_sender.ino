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
 * flymon sender v1.2
 *
 * This is an ambient temperature, humidity, light sensor
 * Will transmit data wirelessely using the RF12 capabilities of a jeenode
 * To upload, use Arduino IDE and select 
 *
 * REMEMBER: define the node_ID to be unique in your network
 * keeping in mind that 1 is used for the receiver
 * 
 *
 * 
 *
 */

#include <JeeLib.h>
#include <PortsSHT11.h>

#define NODE_ID 6    //define the node ID 
#define INTERVAL 60  //refresh interval for sending data, in seconds

#define DEBUG 0      //run in debug mode


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
    #if defined(_AVR_ATtiny84_)
        cli();
        CLKPR = bit(CLKPCE);
        CLKPR = 0;  // div 1, i.e speed up to 8 MHz
        sei();
    #endif
    rf12_initialize(NODE_ID, RF12_868MHZ, 5);
    outData.id = NODE_ID;
}

void loop() {
    
  doMeasure();
  
  while( !rf12_canSend())
      rf12_recvDone();
      
      rf12_sendStart(0, &outData, sizeof outData);
          // set the sync mode to 2 if the fuses are still the Arduino default   
          // mode 3 (full powerdown) can only be used with 258 CK startup fuses    
          rf12_sendWait(2);  
          
          rf12_sleep(RF12_SLEEP);    
          Sleepy::loseSomeTime(INTERVAL*1000);    
          rf12_sleep(RF12_WAKEUP);
}
