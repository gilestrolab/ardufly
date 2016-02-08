//define hardware connection and hard parameters
#define VERSION 1.5
#define masterID 0
#define myID 6

// pin for sensors
#define optoresistor_PIN 15 //A1

// pin for LED mosfet
#define FADE_DELAY 10  // Delay in ms for each percentage fade up/down (10ms = 1s full-range dim)
#define LED_PIN 5

//define hardware and shields used
//#define DEBUG 1
#define USE_SENSIRION 1
#define USE_SD 1
#define USE_RADIO 1

#if defined(USE_RADIO)
  //https://github.com/TMRh20/RF24/archive/master.zip
  #include "RF24.h"
  //https://github.com/TMRh20/RF24Network/archive/master.zip
  #include "RF24Network.h"
  //https://github.com/TMRh20/RF24Mesh/archive/master.zip
  #include "RF24Mesh.h"
  //#include <SPI.h>
#endif

#if defined(USE_SD)
  //https://learn.adafruit.com/adafruit-data-logger-shield
  //https://learn.adafruit.com/adafruit-data-logger-shield/for-the-mega-and-leonardo
  #include <SD.h>  // must use this for MEGA
  File dataFile;
  
  //https://github.com/greiman/SdFat
  //#include <SdFat.h> // this is slightly smaller but I cannot get it to open files
  //SdFat SD;
  //SdFile dataFile;
  // Log file base name.  Must be six characters or less.

  //https://github.com/greiman/Fat16
  //#include <Fat16.h> // this has the smallest imprint but it is FAT16 only, i.e. max 2G

  

  #define SD_CS 10
  #define LOG_FILE "datalog.txt"
#endif

#if defined(USE_DHT)
  //https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTlib
  #include <DHT.h>
  #define HT_PIN 17
  DHT dht; 
#endif

#if defined(USE_SHT1x)
  //https://github.com/practicalarduino/SHT1x
  #include <SHT1x.h>
  #define HT_PIN 17 //A3
  #define clockPin 16 //A2
  SHT1x sht1x(HT_PIN, clockPin);
#endif

#if defined(USE_SENSIRION)
  ///http://playground.arduino.cc/Code/Sensirion
  #include <Sensirion.h>
  #define HT_PIN 17
  #define clockPin 16
  Sensirion sht = Sensirion(HT_PIN, clockPin);
#endif

// To save data in the EEPROM use the following
// http://tronixstuff.com/2011/03/16/tutorial-your-arduinos-inbuilt-eeprom/
#include <EEPROM.h>

// http://www.pjrc.com/teensy/td_libs_Time.html
// http://playground.arduino.cc/Code/Time
//#include <Time.h> 
//#include <Wire.h>
// for RTC see http://www.hobbyist.co.nz/?q=real_time_clock - library from https://github.com/PaulStoffregen/DS1307RTC
#include <DS1307RTC.h> 

// External descriptor of data structure
#include "MyTypes.h"

//Initialising objects and variables

unsigned long counter = 0;
time_t prev_time = 0;

float set_TEMP = 0;
float set_HUM = 0;

byte DD_MODE = 1;  // 0 = DD,  1 = LD, 2 = LL
byte LIGHTS_ON[] = { 9, 00 }; 
byte LIGHTS_OFF[] = { 21, 00 }; 

byte MAX_LIGHT = 100;
byte CURRENT_LIGHT = 0;

bool SEND_REPORT = 1;
byte REPORT_DELAY = 1; // transmission delay in minutes

packageStruct dataPackage = {myID, 0, counter, '-', 0, 0, 0, 0, 0};

#if defined(USE_RADIO)
  /**** Configure the nrf24l01 CE and CS pins ****/
  #define RADIO_CE 8
  #define RADIO_CS 7
  RF24 radio(RADIO_CE, RADIO_CS);
  RF24Network network(radio);
  RF24Mesh mesh(radio, network);
#endif

void setup()
{
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(optoresistor_PIN, INPUT);

#if defined(USE_DHT)  
    dht.setup(HT_PIN); 
#endif

#if defined(USE_SD)
  // disable w5100 SPI
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  // see if the card is present and can be initialized:
  if (!SD.begin(SD_CS, SPI_HALF_SPEED)) {  // arduino UNO
  //if (!SD.begin(SD_CS, 11, 12, 13)) {   // for the arduino MEGA
    Serial.println("SD Error");
    return; // hang everything
  }
  Serial.println("SD OK");
#endif

#if defined(USE_RADIO)
  // disable RF24 SPI
  pinMode(RADIO_CS, OUTPUT);
  digitalWrite(RADIO_CS, HIGH);

  // Connect to the mesh
  mesh.setNodeID(myID);
  Serial.print(F("NRF24L01: "));
  mesh.begin();
  Serial.println(F("OK"));
#endif
  
  //saveTimerValues();
  setSyncProvider(RTC.get);  // get the time from the RTC
  //saveTimerValues(); // uncomment this when creating a new node
  
  // Retrieves Lights ON/OFF timer values and last light status
  setLightsTimer(); 
  // In case of power shutdown, this makes sure that light levels are stored at boot time.
  fadeToLevel(CURRENT_LIGHT);
}


void loop()
{
  REPORT_DELAY = (REPORT_DELAY == 0) ? 1 : REPORT_DELAY;
  float delta = REPORT_DELAY * 1000.0 * 60.0;

  byte hh = hour(); byte mm = minute(); byte ss = second();
  if (( DD_MODE != 0 ) and (hh == LIGHTS_ON[0]) and (mm == LIGHTS_ON[1]) and (ss == 00)) { LightsON(); }
  if (( DD_MODE != 2 ) and ( DD_MODE != 2 ) and (hh == LIGHTS_OFF[0]) and (mm == LIGHTS_OFF[1]) and (ss == 00)) { LightsOFF(); }

#if defined(USE_RADIO)
  mesh.update();
#endif
    
  if ( ( (millis() - prev_time) > delta ) and ( SEND_REPORT ) ) {
    prev_time = millis();

#if defined(USE_RADIO)
    // Just before sending the datapackage will re-register on the network
    // This is in case the master has gone down in the meanwhile - closes bug #2
    mesh.renewAddress();
#endif
    
    // Sending or logging the datapackage
    sendDataPackage('R');
  }

#if defined(USE_RADIO)
    // Check for incoming data from master
    if(network.available()){
        RF24NetworkHeader header;
        network.peek(header);
  
        packageStruct rcvdPackage;
        
        switch(header.type){
          case 'I': 
            network.read(header,&rcvdPackage,sizeof(rcvdPackage)); 
            if ( rcvdPackage.dest_nodeID == myID ) { sendDataPackage('R'); }
            break;
  //        case 'D': 
  //          network.read(header,&rcvdPackage,sizeof(rcvdPackage)); 
  //          if ( rcvdPackage.dest_nodeID == myID ) { debug(); }
  //          break;
          case 'T': 
            network.read(header,&rcvdPackage,sizeof(rcvdPackage)); 
            if ( rcvdPackage.dest_nodeID == myID ) { setRTCTime (rcvdPackage.current_time); }
            break;
          case 'L': 
            network.read(header,&rcvdPackage,sizeof(rcvdPackage));
            if ( rcvdPackage.dest_nodeID == myID ) { fadeToLevel (rcvdPackage.set_light); }
            break;
          case 'F': 
            network.read(header,&rcvdPackage,sizeof(rcvdPackage));
            if ( rcvdPackage.dest_nodeID == myID ) { setInterval (rcvdPackage.set_light); }
            break;
          case 'M': 
            network.read(header,&rcvdPackage,sizeof(rcvdPackage));
            if ( rcvdPackage.dest_nodeID == myID ) { setLightMode (rcvdPackage.set_light); }
            break;
          case '1': 
            network.read(header,&rcvdPackage,sizeof(rcvdPackage)); 
            if ( rcvdPackage.dest_nodeID == myID ) { changeLightsONTimer(rcvdPackage.lights_on); }
            break;
          case '0': 
            network.read(header,&rcvdPackage,sizeof(rcvdPackage)); 
            if ( rcvdPackage.dest_nodeID == myID ) { changeLightsOFFTimer(rcvdPackage.lights_off); }
            break;
          default: network.read(header,0,0); Serial.println(header.type);break;
        }
    }
#endif
}

#if defined(DEBUG) // I am leaving this out for space reasons
void debug(){
  // This works only if the client is actually connected to a computer via the USB
  Serial.print("nodeID: "); Serial.println(myID);
  Serial.print("Time: "); Serial.println(now());
  setLightsTimer();
  Serial.print("L ON: "); Serial.print(LIGHTS_ON[0]); Serial.print(":"); Serial.print(LIGHTS_ON[1]); 
  Serial.print(" / L OFF: "); Serial.print(LIGHTS_OFF[0]); Serial.print(":"); Serial.println(LIGHTS_OFF[1]); 
  Serial.print("Report Interval: "); Serial.println(REPORT_DELAY);
  Serial.print("Send report: "); Serial.println(SEND_REPORT);
  Serial.print("DD Mode: "); Serial.println(DD_MODE);
  
  if ( mesh.checkConnection() ) {  sendDataPackage('R'); }
}
#endif

void setLightMode(int mode)
{
  // 0 = DD 1 = LD 2 = LL
  DD_MODE = mode;
  EEPROM.write(8, DD_MODE);
}


void changeLightsONTimer(time_t lights_on)
{
  LIGHTS_ON[0] = hour(lights_on);
  LIGHTS_ON[1] = minute(lights_on);
  EEPROM.write(1, LIGHTS_ON[0]);
  EEPROM.write(2, LIGHTS_ON[1]);
}

void changeLightsOFFTimer(time_t lights_off)
{
  LIGHTS_OFF[0] = hour(lights_off);
  LIGHTS_OFF[1] = minute(lights_off);
  EEPROM.write(3, LIGHTS_OFF[0]);
  EEPROM.write(4, LIGHTS_OFF[1]);
}


void setLightsTimer()
{
//retrieves values from EEPROM
  LIGHTS_ON[0] = EEPROM.read(1);
  LIGHTS_ON[1] = EEPROM.read(2);
  LIGHTS_OFF[0] = EEPROM.read(3); 
  LIGHTS_OFF[1] = EEPROM.read(4);
  MAX_LIGHT = EEPROM.read(5);
  SEND_REPORT = EEPROM.read(6);
  REPORT_DELAY = EEPROM.read(7);
  DD_MODE = EEPROM.read(8);
  CURRENT_LIGHT = EEPROM.read(9);
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
  EEPROM.write(9, CURRENT_LIGHT);
}

void LightsON(){
  //Serial.println("L ON");
  fadeToLevel(MAX_LIGHT);
}

void LightsOFF(){
  //Serial.println("L OFF");
  fadeToLevel(0);
}

void setRTCTime(time_t time_rcvd)
{
  RTC.set(time_rcvd);
  setTime(time_rcvd);
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

  int delta = ( toLevel - CURRENT_LIGHT ) < 0 ? -1 : 1;

  while ( CURRENT_LIGHT != toLevel ) {
    CURRENT_LIGHT += delta;
    analogWrite( LED_PIN, (int)(CURRENT_LIGHT / 100. * 255) );
    delay( FADE_DELAY );
  }

  EEPROM.write(9, CURRENT_LIGHT);
  sendDataPackage('E');
}

float getTemperature()
{
#if defined(USE_DHT)
    // sensor data for DHT
    return dht.getTemperature();
#endif
#if defined(USE_SHT1x)
    // sensor data for SHT1x library
    return sht1x.readTemperatureC();
#endif
#if defined(USE_SENSIRION)
    // sensor data for Sensiron library
    uint16_t rawData;
    sht.measTemp(&rawData);                // sht.meas(TEMP, &rawData, BLOCK)
    return sht.calcTemp(rawData);
#endif
}

float getHumidity()
{
#if defined(USE_DHT)
    // sensor data for DHT
    return dht.getHumidity();
#endif
#if defined(USE_SHT1x)
    // sensor data for SHT1x library
    return sht1x.readHumidity();
#endif
#if defined(USE_SENSIRION)
    // sensor data for Sensiron library
    uint16_t rawData;
    sht.measTemp(&rawData);                // sht.meas(TEMP, &rawData, BLOCK)
    float temperature = sht.calcTemp(rawData);
    sht.measHumi(&rawData);                // sht.meas(HUMI, &rawData, BLOCK)
    return sht.calcHumi(rawData, temperature);
#endif
}

void sendDataPackage(char cmd){

    // collect all the data in one package 
    dataPackage.orig_nodeID = myID;
    dataPackage.dest_nodeID = masterID;
    dataPackage.counter = counter++;
    dataPackage.cmd = cmd; //R Report, E event
    dataPackage.current_time = now();
    
    dataPackage.temp = getTemperature();
    dataPackage.hum = getHumidity();
    dataPackage.light = analogRead(optoresistor_PIN); 

    // set data
    dataPackage.set_temp = set_TEMP; // not yet implemented
    dataPackage.set_hum = set_HUM; // not yet implemented
    dataPackage.set_light = CURRENT_LIGHT;
    
    //light timer data
    dataPackage.lights_on = (LIGHTS_ON[0] * 3600.0) + (LIGHTS_ON[1] * 60.0);
    dataPackage.lights_off = (LIGHTS_OFF[0] * 3600.0) + (LIGHTS_OFF[1] * 60.0);
    dataPackage.dd_mode = DD_MODE;

#if defined(USE_RADIO)
    Serial.print(F("RF: "));
     if (!mesh.write( &dataPackage, dataPackage.cmd, sizeof(dataPackage), masterID )){
       Serial.println(F("FAIL"));
     } else { Serial.println("OK"); }
#endif

#if defined(USE_SD)
    Serial.print("File: ");
    dataFile = SD.open(LOG_FILE, FILE_WRITE);
    if (!dataFile) {
    //if (!dataFile.open(fileName, O_CREAT | O_WRITE | O_EXCL)) {
      Serial.println("ERR");
    }
    else {
      
     dataFile.print(myID); dataFile.print(",");
     dataFile.print(counter); dataFile.print(",");
     dataFile.print(cmd); dataFile.print(",");
     dataFile.print(dataPackage.current_time); dataFile.print(",");
     dataFile.print(dataPackage.temp); dataFile.print(",");
     dataFile.print(dataPackage.hum); dataFile.print(",");
     dataFile.print(dataPackage.light); dataFile.print(",");
     dataFile.print(dataPackage.set_temp); dataFile.print(",");
     dataFile.print(dataPackage.set_hum); dataFile.print(",");
     dataFile.print(dataPackage.set_light); dataFile.print(",");
     dataFile.print(dataPackage.lights_on); dataFile.print(",");
     dataFile.print(dataPackage.lights_off); dataFile.print(",");
     dataFile.println(dataPackage.dd_mode);
     dataFile.close();
     
     Serial.println("OK");
    }
#endif     
}
