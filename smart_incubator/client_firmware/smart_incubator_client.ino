//https://github.com/TMRh20/RF24/archive/master.zip
#include "RF24.h"
//https://github.com/TMRh20/RF24Network/archive/master.zip
#include "RF24Network.h"
//https://github.com/TMRh20/RF24Mesh/archive/master.zip
#include "RF24Mesh.h"
#include <SPI.h>
#include <EEPROM.h>

//https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTlib
#include <DHT.h>

// http://www.pjrc.com/teensy/td_libs_Time.html
// http://playground.arduino.cc/Code/Time
#include <Time.h> 
#include <Wire.h>

// for RTC see http://www.hobbyist.co.nz/?q=real_time_clock
// https://github.com/PaulStoffregen/DS1307RTC
#include <DS1307RTC.h> 

// External descriptor of data structure
#include "MyTypes.h"

// To save data in the EEPROM use the following
// http://tronixstuff.com/2011/03/16/tutorial-your-arduinos-inbuilt-eeprom/
#include <EEPROM.h>

//defines
#define VERSION 1.0
#define nodeID 5
#define masterID 0

// pin for sensors
#define DHT_PIN 17
#define light_PIN 16

// pin for LED mosfet
#define FADE_DELAY 10  // Delay in ms for each percentage fade up/down (10ms = 1s full-range dim)
#define LED_PIN 5

//Initialising objects and variables
DHT dht; 

int currentLevel = 0;
unsigned long counter = 0;
unsigned long prev_time = 0;

bool DD_MODE = 0;
byte LIGHTS_ON[] = { 9, 00 }; 
byte LIGHTS_OFF[] = { 21, 00 }; 
byte MAX_LIGHT = 100;
byte CURRENT_LIGHT_STATUS = 0;

bool SEND_REPORT = 1;
byte REPORT_DELAY = 1; // transmission delay in minutes

packageStruct dataPackage = {masterID, 0, counter, '-', 0, 0, 0, 0, 0};


/**** Configure the nrf24l01 CE and CS pins ****/
RF24 radio(8, 7);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

void setup()
{
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(light_PIN, INPUT);
  dht.setup(DHT_PIN); 

  //saveTimerValues();
  setSyncProvider(RTC.get);  // get the time from the RTC
  
  // Retrieves Lights ON/OFF timer values and last light status
  setLightsTimer(); 

  //radio.setRetries(15,15);
  //radio.setPayloadSize(8);

  mesh.setNodeID(nodeID);
  // Connect to the mesh
  Serial.print(F("Connecting to the mesh..."));
  mesh.begin();
  Serial.println(F(" Connected"));

  // In case of power shutdown, this makes sure that light levels are stored at boot time.
  fadeToLevel(CURRENT_LIGHT_STATUS);

}


void loop()
{
 //delay(dht.getMinimumSamplingPeriod());
  float delta = REPORT_DELAY * 1000.0 * 60.0;

  byte hh = hour(); byte mm = minute(); byte ss = second();
  if (!(DD_MODE) and (hh == LIGHTS_ON[0]) and (mm == LIGHTS_ON[1]) and (ss == 00)) { LightsON(); }
  if (!(DD_MODE) and (hh == LIGHTS_OFF[0]) and (mm == LIGHTS_OFF[1]) and (ss == 00)) { LightsOFF(); }
 
  mesh.update();
  if ( ( (millis() - prev_time) > delta ) and ( SEND_REPORT ) ) {
    prev_time = millis();
    Serial.print("T: ");
    sendDataPackage('R');
  }

  // Check for incoming data from master
  if(network.available()){
      RF24NetworkHeader header;
      network.peek(header);

      packageStruct rcvdPackage;
      
      switch(header.type){
        case 'I': 
          network.read(header,&rcvdPackage,sizeof(rcvdPackage)); 
          if ( rcvdPackage.dest_nodeID == nodeID ) { sendDataPackage('R'); }
          break;
        case 'D': 
          network.read(header,&rcvdPackage,sizeof(rcvdPackage)); 
          if ( rcvdPackage.dest_nodeID == nodeID ) { debug(); }
          break;
        case 'T': 
          network.read(header,&rcvdPackage,sizeof(rcvdPackage)); 
          if ( rcvdPackage.dest_nodeID == nodeID ) { setRTCTime (rcvdPackage.unixTimeStamp); }
          break;
        case 'L': 
          network.read(header,&rcvdPackage,sizeof(rcvdPackage));
          if ( rcvdPackage.dest_nodeID == nodeID ) { fadeToLevel (rcvdPackage.led_level); }
          break;
        case 'F': 
          network.read(header,&rcvdPackage,sizeof(rcvdPackage));
          if ( rcvdPackage.dest_nodeID == nodeID ) { setInterval (rcvdPackage.led_level); }
          break;
        case 'M': 
          network.read(header,&rcvdPackage,sizeof(rcvdPackage));
          if ( rcvdPackage.dest_nodeID == nodeID ) { setLightMode (rcvdPackage.led_level); }
          break;
        case '1': 
          network.read(header,&rcvdPackage,sizeof(rcvdPackage)); 
          if ( rcvdPackage.dest_nodeID == nodeID ) { changeLightsONTimer(rcvdPackage.unixTimeStamp); }
          break;
        case '0': 
          network.read(header,&rcvdPackage,sizeof(rcvdPackage)); 
          if ( rcvdPackage.dest_nodeID == nodeID ) { changeLightsOFFTimer(rcvdPackage.unixTimeStamp); }
          break;
        default: network.read(header,0,0); Serial.println(header.type);break;
      }
  }
 
}

void debug(){
  // This works only if the client is actually connected to a computer via the USB
  Serial.print("nodeID: "); Serial.println(nodeID);
  Serial.print("Current time: "); Serial.println(now());
  setLightsTimer();
  Serial.print("Lights ON: "); Serial.print(LIGHTS_ON[0]); Serial.print(":"); Serial.print(LIGHTS_ON[1]); 
  Serial.print(" / Lights OFF: "); Serial.print(LIGHTS_OFF[0]); Serial.print(":"); Serial.println(LIGHTS_OFF[1]); 
  Serial.print("Report Interval: "); Serial.println(REPORT_DELAY);
  Serial.print("Send report: "); Serial.println(SEND_REPORT);
  Serial.print("DD Mode: "); Serial.println(DD_MODE);
  
  sendDataPackage('R');
}

void setLightMode(int mode)
{
  DD_MODE = mode;
  EEPROM.write(8, DD_MODE);
  Serial.print("DD_MODE set to "); Serial.println(DD_MODE);
}


void changeLightsONTimer(long unixTimeStamp)
{
  time_t tt = unixTimeStamp;
  LIGHTS_ON[0] = hour(tt);
  LIGHTS_ON[1] = minute(tt);
  EEPROM.write(1, LIGHTS_ON[0]);
  EEPROM.write(2, LIGHTS_ON[1]);
}

void changeLightsOFFTimer(long unixTimeStamp)
{
  time_t tt = unixTimeStamp;
  LIGHTS_OFF[0] = hour(tt);
  LIGHTS_OFF[1] = minute(tt);
  EEPROM.write(3, LIGHTS_OFF[0]);
  EEPROM.write(4, LIGHTS_OFF[1]);
}


void setLightsTimer()
{
//retrieves values to EEPROM
  LIGHTS_ON[0] = EEPROM.read(1);
  LIGHTS_ON[1] = EEPROM.read(2);
  LIGHTS_OFF[0] = EEPROM.read(3); 
  LIGHTS_OFF[1] = EEPROM.read(4);
  MAX_LIGHT = EEPROM.read(5);
  SEND_REPORT = EEPROM.read(6);
  REPORT_DELAY = EEPROM.read(7);
  DD_MODE = EEPROM.read(8);
  CURRENT_LIGHT_STATUS = EEPROM.read(9);
}

void saveTimerValues()
{
//saves values to EEPROM
  EEPROM.write(1, LIGHTS_ON[0]);
  EEPROM.write(2, LIGHTS_ON[1]);
  EEPROM.write(3, LIGHTS_OFF[0]);
  EEPROM.write(4, LIGHTS_OFF[1]);
  EEPROM.write(5, MAX_LIGHT);
  EEPROM.write(6, SEND_REPORT);
  EEPROM.write(7, REPORT_DELAY);
  EEPROM.write(8, DD_MODE);
  EEPROM.write(9, CURRENT_LIGHT_STATUS);
}

void LightsON(){
  Serial.println("Switching Lights on now");
  fadeToLevel(MAX_LIGHT);
}

void LightsOFF(){
  Serial.println("Switching Lights off now");
  fadeToLevel(0);
}

void setRTCTime(long unixTimeStamp)
{
  RTC.set(unixTimeStamp);
  setTime(unixTimeStamp);
  
}

void setInterval(byte interval)
{
  REPORT_DELAY = interval;
  SEND_REPORT = ( REPORT_DELAY > 0 );
  EEPROM.write(6, SEND_REPORT);
  EEPROM.write(7, REPORT_DELAY);
}

/***
 *  This method provides a graceful fade up/down effect
 */
void fadeToLevel( int toLevel ) {

  int delta = ( toLevel - currentLevel ) < 0 ? -1 : 1;
  while ( currentLevel != toLevel ) {
    currentLevel += delta;
    analogWrite( LED_PIN, (int)(currentLevel / 100. * 255) );
    delay( FADE_DELAY );
  }

  CURRENT_LIGHT_STATUS = toLevel;
  EEPROM.write(9, CURRENT_LIGHT_STATUS);
  
  sendDataPackage('E');
}

void sendDataPackage(char cmd){
    dataPackage.orig_nodeID = nodeID;
    dataPackage.dest_nodeID = masterID;
    dataPackage.counter = counter++;
    dataPackage.cmd = cmd; //R Report, T setTime, L SetLight, 0 / 1 setTimer, E event
    dataPackage.unixTimeStamp = now();
    dataPackage.temp = dht.getTemperature();
    dataPackage.hum = dht.getHumidity();
    dataPackage.light = analogRead(light_PIN); 
    dataPackage.led_level = currentLevel;
    
    Serial.print(F("Now sending Package "));
     if (!mesh.write( &dataPackage, dataPackage.cmd, sizeof(dataPackage), masterID )){
       Serial.println(F("failed"));
     } else { Serial.println("OK"); }
}
