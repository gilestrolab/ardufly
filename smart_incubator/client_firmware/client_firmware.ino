//Without SD
//2016-09-16
//Sketch uses 25,592 bytes (79%) of program storage space. Maximum is 32,256 bytes.
//Global variables use 1,655 bytes (80%) of dynamic memory, leaving 393 bytes for local variables. Maximum is 2,048 bytes.

//define hardware connection and hard parameters
#define VERSION 2.3
#define masterID 0
#define myID 15

//still not implemented
#define BUTTON_PIN 2 /// THIS IS ACTUALLY USED BY TEC DIR

//bicolor LED (center pin goes to cathode)
#define GREEN_PIN 3
#define RED_PIN 4

// pin for sensors
#define optoresistor_PIN 15 //A1

// pin for LED mosfet
#define FADE_DELAY 10  // Delay in ms for each percentage fade up/down (10ms = 1s full-range dim)
#define LED_PIN 10 // Luis' MOSFETS are on 5,6,9,10 - 10 is shared with SD // Wine Cooler is on 6 // Regular Chicks on 5

//#define HUM_PIN 9

//pin for PELTIER TEC
#define PELTIER_PWM 5 // what is the frequency of pin 6? pin 9 has a pwm frequency of 490hz
#define PELTIER_DIR 2

//define hardware and shields used
#define USE_SENSIRION
#define USE_RADIO
#define USE_TEC
//#define USE_TEC_PWM
//#define USE_SD 1  // we do not use SD on regular UNOs because a) we don't need it and b) it takes too much memory
//#define LOCAL_SERIAL

#define CMD 'C'
#define REPORT 'R'
#define EVENT 'E'


#if defined(USE_RADIO)
  //https://github.com/TMRh20/RF24/archive/master.zip
  #include "RF24.h"
  //https://github.com/TMRh20/RF24Network/archive/master.zip
  #include "RF24Network.h"
  //https://github.com/TMRh20/RF24Mesh/archive/master.zip
  #include "RF24Mesh.h"
  #include <SPI.h>
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
  // http://playground.arduino.cc/Code/Sensirion
  // Note that a modification to the code is needed to work with 1.0 - see webpage
  #include <Sensirion.h>
  #define HT_PIN 17
  #define clockPin 16
  Sensirion sht = Sensirion(HT_PIN, clockPin);
#endif

// To save data in the EEPROM use the following
// http://tronixstuff.com/2011/03/16/tutorial-your-arduinos-inbuilt-eeprom/
#include <EEPROM.h>

#if defined(LOCAL_SERIAL)
  #include <SerialCommand.h>
  SerialCommand sCmd;
#endif

// http://www.pjrc.com/teensy/td_libs_Time.html
// http://playground.arduino.cc/Code/Time
#include <Time.h> 
#include <Wire.h>
// for RTC see http://www.hobbyist.co.nz/?q=real_time_clock 
// libraries from https://github.com/PaulStoffregen/DS1307RTC and https://github.com/PaulStoffregen/Time
#include <DS1307RTC.h> 

// External descriptor of data structure
#include "MyTypes.h"

//Experimenting with watchdog
#include <avr/wdt.h>

//Initialising objects and variables

unsigned long counter = 0;
time_t prev_time = 0;

packageStruct dataPackage = {myID, 0, counter, REPORT, 0, 0, 0, 0, 0};

configuration cfg = {
    540, // lights on minute ( hh * 60 + mm ) 09:00
    1260, // lights off minute                  21:00

    100, // cfg.max_light
    1, // cfg.dd_mode  0 = DD,  1 = LD, 2 = LL, 3 = DL
    
    true, // cfg.send_report
    5, // cfg.report_delay in minutes

    22.0, // cfg.set_temp
    65.0 // cfg.set_hum 
};

environment env = {0, 0, 0}; 

float DELTA = cfg.report_delay * 1000.0 * 60.0; // delta between reports, in milliseconds
int CURRENT_LIGHT = 0 ;

time_t last_sensor_reading = 0;
int MIN_SENSOR_PAUSE = 5000; // to avoid self heating of the probe we don't read more often than once every 5 seconds


#if defined(USE_RADIO)
/**** Configure the nrf24l01 CE and CS pins ****/
#define RADIO_CE 8
#define RADIO_CS 7
RF24 radio(RADIO_CE, RADIO_CS);
RF24Network network(radio);
RF24Mesh mesh(radio, network);
#define MESH_RENEWAL_TIMEOUT 5000 // changing timeout from 60.000 ms to 5.000ms
#endif


void setup()
{  
  
  Serial.begin(115200);

#if defined(LOCAL_SERIAL)  
  setupSerialCommands();
#endif
  
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
  
  Serial.print(F("Starting NRF24L01: "));
  if ( mesh.begin(MESH_DEFAULT_CHANNEL, RF24_1MBPS, MESH_RENEWAL_TIMEOUT) ) 
    Serial.println(F("Done"));
  else
    Serial.println("Error");
#endif
  
  //saveConfiguration(); // Enable this on first update only, then reupload
  setSyncProvider(RTC.get);  // get the time from the RTC
  
  // Retrieves Lights ON/OFF timer values and last light status
  loadConfiguration();
  debug();
  
#if defined(USE_TEC)
  // Initialize the PWM and DIR pins as digital outputs.
  pinMode(PELTIER_PWM, OUTPUT);
  pinMode(PELTIER_DIR, OUTPUT);
#endif

  wdt_enable(WDTO_8S);

}

#if defined(LOCAL_SERIAL)
void serial_setTime (){
  //set time on the RTC module of the node - called from serial port
  char *arg;
  time_t unixstamp = 0;

  arg = sCmd.next();
  if (arg != NULL) {
    unixstamp = atol(arg);
  }

  setRTCTime(unixstamp);
}

void serial_setLight (){
  // turn the node lights at value 0-100% - called from serial port
  char *arg;
  int light_level = 0;

  arg = sCmd.next();
  if (arg != NULL) {
    light_level = atoi(arg);
  }

  fadeToLevel(light_level);
}

void serial_setLightsONTimer(){
  // Changes the HH:MM for lights on - Called from serial port
  char *arg;
  time_t lights_on = 0;

  arg = sCmd.next();
  if (arg != NULL) {
    lights_on = atol(arg);
  }

  changeLightsONTimer(lights_on);
}

void serial_setLightsOFFTimer(){
  // Changes the HH:MM for lights off - called from serial port
  char *arg;
  time_t lights_off = 0;

  arg = sCmd.next();
  if (arg != NULL) {
    lights_off = atol(arg);
  }

  changeLightsOFFTimer(lights_off);
}

void serial_setLightMode(){
  // Change the light mode 
  // 0 DD, 1 LD, 2 LL
  char *arg;
  int mode = 1;

  arg = sCmd.next();
  if ((arg != NULL) and (strcmp (arg,"DD") == 0)){ mode = 0; };
  if ((arg != NULL) and (strcmp (arg,"LD") == 0)){ mode = 1; };
  if ((arg != NULL) and (strcmp (arg,"LL") == 0)){ mode = 2; };
  if ((arg != NULL) and (strcmp (arg,"DL") == 0)){ mode = 3; };
  if ((arg != NULL) and (strcmp (arg,"00") == 0)){ mode = 4; };

  setLightMode(mode);
}

void serial_setTemperature(){
    
  // set the current Temperature on node
  char *arg;
  float set_TEMP = 0;

  arg = sCmd.next();
  if (arg != NULL) {
    set_TEMP = atof(arg);
  }

  setTemperature(set_TEMP);
}



void serial_setHumidity(){
  // set the current Humidity on node
  char *arg;
  float set_HUM = 0;
  
  arg = sCmd.next();
  if (arg != NULL) {
    set_HUM = atof(arg);
  }

  setHumidity(set_HUM);
}

void serial_setMaxLight (){
  // Set the maximum level of lights allowed
  char *arg;
  int light_level = 0;
  
  arg = sCmd.next();
  if (arg != NULL) {
    light_level = atoi(arg);
  }
  
  setMaxLight(light_level);
}

void setupSerialCommands() {
//Defines the available serial commands
  sCmd.addCommand("?",     printHelp);
  sCmd.addCommand("T",     serial_setTime);
  sCmd.addCommand("L",     serial_setLight);
  sCmd.addCommand("I",     debug);
  sCmd.addCommand("1",     serial_setLightsONTimer);
  sCmd.addCommand("0",     serial_setLightsOFFTimer);
  sCmd.addCommand("M",     serial_setLightMode);
  sCmd.addCommand("C",     serial_setTemperature);
  sCmd.addCommand("H",     serial_setHumidity);
  sCmd.addCommand("X",     serial_setMaxLight);
  sCmd.setDefaultHandler(printError);      // Handler for command that isn't matched  (says "What?")
}

void printHelp() {
  Serial.println("HELP               Print this help message");
  Serial.println("M DD               Set light mode to DD, LD, LL, DL, 00");
  Serial.println("T timestamp        Set time");
  Serial.println("L light (%)        Set light levels");
  Serial.println("I                  Print info");
  Serial.println("1 timestamp        Set time for Lights ON");
  Serial.println("0 timestamp        Set time for Lights OFF");
  Serial.println("C temperature      Set temperature");
  Serial.println("H humidity         Set humidity");
  Serial.println("X light            Set max light");
}

void printError(const char *command) {
  // This gets set as the default handler, and gets called when no other command matches.
  Serial.println("Command not valid");
}

#endif //LOCAL_SERIAL

bool lightIsON()
{
    return CURRENT_LIGHT > 0;
}

bool lightIsOFF()
{
    return not lightIsON();
}


void loop()
{

    wdt_reset();

#if defined(USE_TEC)
    // we do this continously but the freshness of the getTemperature result is 
    // controlled by the MIN_SENSOR_PAUSE variable
    float SLOWING = 0.5;
    float delta_T = getTemperature() - cfg.set_temp;
    
    int power = ( abs(delta_T) / SLOWING ) * 100.0;
    //int power = 100; //always full power
    
    abs(power) > 100 ? power = 100 : power = abs(power);
    setPeltier(power, (abs(delta_T)/delta_T ) );  //
#endif
  
  //Makes sure the light is in the right status
  int minute_of_the_day = hour() * 60 + minute();
  
  switch(cfg.dd_mode) {
      
      case 0: //DD
        if ( lightIsON() ) { LightsOFF(); }
        break;
      case 1: //LD
        if (lightIsOFF() && minute_of_the_day >= cfg.lights_on && minute_of_the_day < cfg.lights_off) { LightsON(); }
        if (lightIsON() && minute_of_the_day >= cfg.lights_off) { LightsOFF(); }
        break;
      case 2: //LL
        if ( lightIsOFF() ) { LightsON(); }
        break;
      case 3: //DL
        if (lightIsON() && minute_of_the_day >= cfg.lights_off && minute_of_the_day < cfg.lights_on) { LightsOFF(); }
        if (lightIsOFF() && minute_of_the_day >= cfg.lights_on) { LightsON(); }
        break;
      default:
        // do nothing
        break;
    }

#if defined(USE_RADIO)
  mesh.update();
#endif
    
  if ( (millis() - prev_time) > DELTA or prev_time == 0 )  {
    prev_time = millis();

#if defined(USE_RADIO)
    // Just before sending the datapackage will re-register on the network
    // This is in case the master has gone down in the meanwhile - closes bug #2
    mesh.renewAddress(MESH_RENEWAL_TIMEOUT);
#endif
    
    // Sending or logging the datapackage
    if (cfg.send_report)
        sendDataPackage(REPORT);
  }

#if defined(USE_RADIO)
    // Check for incoming data from master
    if(network.available()){
        RF24NetworkHeader header;
        network.peek(header);
  
        packageStruct rcvdPackage;
        
        if (header.type == CMD) {
            
            network.read(header,&rcvdPackage,sizeof(rcvdPackage));
            Serial.print("received command: ");
            Serial.println(rcvdPackage.cmd);
           
            switch(rcvdPackage.cmd) {
                case 'I':
                    sendDataPackage(REPORT);
                    break;
                case 'T':
                    setRTCTime (rcvdPackage.current_time);
                    break;
                case 'L':
                    fadeToLevel (rcvdPackage.set_light);
                    break;
                case 'F':
                    setTransmissionInterval (rcvdPackage.set_light);
                    break;
                case 'M':
                    setLightMode (rcvdPackage.set_light);
                    break;
                case '1':
                    changeLightsONTimer(rcvdPackage.lights_on);
                    break;
                case '0':
                    changeLightsOFFTimer(rcvdPackage.lights_off);
                    break;
                case 'C':
                    setTemperature(rcvdPackage.set_temp);
                    break;
                case 'H':
                    setHumidity(rcvdPackage.set_hum);
                    break;
                case 'X':
                    setMaxLight(rcvdPackage.set_light);
                    break;
                case 'D':
                    debug();
                default:
                    network.read(header,0,0); Serial.println(header.type);
                    break;
                }  // end switch case
        } // end if cmd
    } //endif network
#endif // use radio

#if defined(LOCAL_SERIAL)
sCmd.readSerial(); 
#endif
 
} // end of loop

void debug(){
  // This works only if the client is actually connected to a computer via the USB
  Serial.print("nodeID: "); Serial.println(myID);
  Serial.print("Time: "); Serial.println(now());
  Serial.print("Lights ON: "); Serial.println(cfg.lights_on); 
  Serial.print("Lights OFF: "); Serial.println(cfg.lights_off); 
  Serial.print("Report Interval: "); Serial.println(cfg.report_delay);
  Serial.print("Send report: "); Serial.println(cfg.send_report);
  Serial.print("Temperature: "); Serial.print(env.temperature);
  Serial.print("/"); Serial.println(cfg.set_temp);
  Serial.print("Humidity: "); Serial.print(env.humidity);
  Serial.print("/"); Serial.println(cfg.set_hum);
  Serial.print("Light: "); Serial.print(CURRENT_LIGHT);
  Serial.print("/"); Serial.print(cfg.max_light);
  Serial.print(" - "); Serial.println(env.light);
  Serial.print("Light Mode: "); Serial.println(cfg.dd_mode);
}

void setLightMode(int mode)
{
  // 0 = DD 1 = LD 2 = LL 3 = DL, 4 = Manual
  cfg.dd_mode = mode;
  saveConfiguration();
  sendDataPackage(EVENT);
}


void changeLightsONTimer(time_t lights_on)
{
  cfg.lights_on = hour(lights_on) * 60 + minute(lights_on);
  saveConfiguration();
  sendDataPackage(EVENT);
}

void changeLightsOFFTimer(time_t lights_off)
{
  cfg.lights_off = hour(lights_off) * 60 + minute(lights_off);
  saveConfiguration();
  sendDataPackage(EVENT);
}

void loadConfiguration()
{
// on the very first flash, byte 9 is set to 255
// we use this to save the default values to the arduino
  Serial.println("Load Configuration from memory");
  if (EEPROM.read(0) != 1) { saveConfiguration(); } 
  EEPROM.get(1, cfg);
}

void saveConfiguration()
{
//saves values to EEPROM

  Serial.println("==Saving:==");
  debug();
  EEPROM.write(0, 1);
  EEPROM.put(1, cfg);
}

void LightsON(){
  fadeToLevel(cfg.max_light);
  sendDataPackage(EVENT);
}

void LightsOFF(){
  fadeToLevel(0);
  sendDataPackage(EVENT);
}

void setRTCTime(time_t time_rcvd)
{
  RTC.set(time_rcvd);
  setTime(time_rcvd);
  sendDataPackage(EVENT);
}

void setTransmissionInterval(byte interval)
{
  (interval < 1) ? cfg.report_delay = 1 : cfg.report_delay = interval;
  cfg.send_report = ( cfg.report_delay > 0 );
  saveConfiguration();
}

void setTemperature(float temp)
{
  cfg.set_temp = temp;
  saveConfiguration();
  sendDataPackage(EVENT);
}

void setHumidity(float hum)
{
  cfg.set_hum = hum;
  saveConfiguration();
  sendDataPackage(EVENT);
}

void setMaxLight(int light)
{
  cfg.max_light = light;
  saveConfiguration();
  sendDataPackage(EVENT);
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
}

/*
 * This regulates the output of the PELTIER
 * PWM 0-100
 * DIR +1 / -1
 * DIR OR PWM 0 will halt peltier
 */
void setPeltier(int PWM, int direction)
{

#if defined(USE_TEC_PWM)
    analogWrite(PELTIER_PWM, (int)(PWM / 100. * 255));
#else
    analogWrite(PELTIER_PWM, 255);
#endif

    if (direction > 0) { digitalWrite(PELTIER_DIR, HIGH ); }
    if (direction < 0) { digitalWrite(PELTIER_DIR, LOW ); }
    
    if (direction == 0) { 
        digitalWrite(PELTIER_DIR, LOW ); 
        analogWrite(PELTIER_PWM, 0);
        }
}

void updateEnvValues() {
    
    // if we asked recently, we return the last values
    if (( millis() - last_sensor_reading >= MIN_SENSOR_PAUSE ) or last_sensor_reading == 0 ) {
    
        last_sensor_reading = millis();
        
    #if defined(USE_DHT)
        env.temperature =  dht.getTemperature();
        env.humidity =  dht.getHumidity();
    #endif
    #if defined(USE_SHT1x)
        env.temperature = sht1x.readTemperatureC();
        env.humidity = sht1x.readHumidity();
    #endif
    #if defined(USE_SENSIRION)
        uint16_t rawData;
        sht.measTemp(&rawData);                // sht.meas(TEMP, &rawData, BLOCK)
        env.temperature = sht.calcTemp(rawData);
        sht.measHumi(&rawData);                // sht.meas(HUMI, &rawData, BLOCK)
        env.humidity = sht.calcHumi(rawData, env.temperature);
    #endif
        env.light = analogRead(optoresistor_PIN);
    }

}


float getTemperature()
{
    updateEnvValues();
    return env.temperature;
}

float getHumidity()
{
    updateEnvValues();
    return env.humidity;
}

void sendDataPackage(char header_type){

    // collect all the data in one package 
    dataPackage.orig_nodeID = myID;
    dataPackage.dest_nodeID = masterID;
    dataPackage.counter = counter++;
    dataPackage.cmd = header_type; //R Report, E event, C command
    dataPackage.current_time = now();
    
    updateEnvValues();
    dataPackage.temp = env.temperature;
    dataPackage.hum = env.humidity;
    dataPackage.light = env.light; 

    // set data
    dataPackage.set_temp = cfg.set_temp;
    dataPackage.set_hum = cfg.set_hum; // not yet implemented
    dataPackage.set_light = CURRENT_LIGHT;
    
    //light timer data
    dataPackage.lights_on = (cfg.lights_on * 60.0); // in seconds
    dataPackage.lights_off = (cfg.lights_off * 60.0); // in seconds
    dataPackage.dd_mode = cfg.dd_mode;

    Serial.println("==Sending:==");
    debug();

#if defined(USE_RADIO)
    Serial.print(F("RF: "));
     if (!mesh.write( &dataPackage, header_type, sizeof(dataPackage), masterID )){
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
