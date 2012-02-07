#include <JeeLib.h>

#define pinLight1 0
#define pinLight2 1
#define pinTemp1 2
#define pinTemp2 3
#define aref_voltage 3.3
#define DEBUG  1

struct {
    byte light1;  // light sensor: 0..255
    byte light2;  // motion detector: 0..1
    int temp1   :10; // temperature: -500..+500 (tenths)
    int temp2   :10; // temperature: -500..+500 (tenths)
    byte lobat :1;  // supply voltage dropped under 3.1V: 0..1
} payload;

// this must be added since we're using the watchdog for low-power waiting
ISR(WDT_vect) {Sleepy::watchdogEvent();}

//reads light analog input and transforms it to byte
byte lightRead(int lightPin) {
  byte light = analogRead(lightPin) / 4;
  return light;
}

// reads temperature analog input and transforms it to Centigrade
float tempRead(int tempPin) {
  int tempReading = analogRead(tempPin);  
  // converting that reading to voltage, which is based off the reference voltage
  float voltage = tempReading * aref_voltage / 1024; 
   // now print out the temperature
  float temperatureC = (voltage - 0.5) * 100 ; 
  return temperatureC;
}

static void serialFlush () {
    #if ARDUINO >= 100
        Serial.flush();
    #endif  
    delay(2); // make sure tx buf is empty before going back to sleep
}

void doMeasure() {
  payload.lobat = rf12_lowbat();
  
  payload.light1 = lightRead(pinLight1);
  payload.light2 = lightRead(pinLight2);
  
  payload.temp1 = tempRead(pinTemp1);
  payload.temp2 = tempRead(pinTemp2);
  
  #if DEBUG
    Serial.println("Reading ");
    Serial.println(payload.light1);
//   Serial.println(payload.light2);
//   Serial.println(payload.temp1);
//   Serial.println(payload.temp2);
//   Serial.println(payload.lobat);
    serialFlush();
  #endif  
  
}

void setup() {
  Serial.begin(57600);
  Serial.println( "Transmission started.");
  serialFlush();
  #if defined(_AVR_ATtiny84_)
    cli();
    CLKPR = bit(CLKPCE);
    CLKPR = 0;  // div 1, i.e speed up to 8 MHz
    sei();
  #endif
    rf12_initialize(18, RF12_868MHZ, 5);
}

void loop() {
    
  doMeasure();
  
  while( !rf12_canSend())
      rf12_recvDone();
      
      rf12_sendStart(0, &payload, sizeof payload);
          // set the sync mode to 2 if the fuses are still the Arduino default   
          // mode 3 (full powerdown) can only be used with 258 CK startup fuses    
          rf12_sendWait(2);  
          
          rf12_sleep(RF12_SLEEP);    
          Sleepy::loseSomeTime(10000);    
          rf12_sleep(RF12_WAKEUP);
}
