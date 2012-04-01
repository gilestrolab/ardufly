#include <JeeLib.h>
#include <PortsSHT11.h>

#define NODE_ID 5    //define the node ID 
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
