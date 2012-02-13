#include <JeeLib.h>

#define NODE_ID 1    //define the node ID - DO NOT CHANGE

struct {
    byte id :7;  // the NODE_ID
    long event; // a growing number counting the events
    float temp; // the temperature measured on the SHT sensor
    float humi; // the humidity measured on the SHT sensor
    byte light; // light measured on the analog sensor
    byte temp_out; // the temperature measured on the TMP36 sensor (optional)
    byte lobat :1;  // supply voltage dropped under 3.1V: 0..1
} inData;

struct {
  byte id : 7; // destination NODE_ID
  byte relay_a :1; //turn relay A on or off
  byte relay_b :1; //turn relay B on or off
} outData;

byte pendingOutput;
MilliTimer sendTimer;

static void serialFlush () {
    #if ARDUINO >= 100
        Serial.flush();
    #endif  
    delay(2); // make sure tx buf is empty before going back to sleep
}

void setup () {
    Serial.begin(57600);
    Serial.println( "Ready.");
    serialFlush();
    
    #if defined(_AVR_ATtiny84_)
        cli();
        CLKPR = bit(CLKPCE);
        CLKPR = 0;  // div 1, i.e speed up to 8 MHz
        sei();
    #endif
    
    rf12_initialize(NODE_ID, RF12_868MHZ, 5);
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

static byte produceOutData () {
    //leave it for future mutual communication
    return 0;
}

void loop () {
    if (rf12_recvDone() && rf12_crc == 0 && rf12_len == sizeof inData) {
        memcpy(&inData, (byte*) rf12_data, sizeof inData);
        // optional: rf12_recvDone(); // re-enable reception right away
        consumeInData();
    }

    if (sendTimer.poll(100))
        pendingOutput = produceOutData();

    if (pendingOutput && rf12_canSend()) {
        rf12_sendStart(0, &outData, sizeof outData, 2);
        // optional: rf12_sendWait(2); // wait for send to finish
        pendingOutput = 0;
    }
}

