//Without SD
//2016-03-22
//Sketch uses 22,884 bytes (70%) of program storage space. Maximum is 32,256 bytes.
//Global variables use 1,179 bytes (57%) of dynamic memory, leaving 869 bytes for local variables. Maximum is 2,048 bytes.

//define hardware connection and hard parameters
#define VERSION 2.0
#define masterID 0
#define myID 2

// pin for sensors
#define optoresistor_PIN 15 //A1

// pin for LED mosfet
#define FADE_DELAY 10  // Delay in ms for each percentage fade up/down (10ms = 1s full-range dim)
#define LED_PIN 5 // Luis' MOSFETS are on 5,6,9,10 - 10 is shared with SD

//#define HUM_PIN 5

//pin for PELTIER TEC
#define PELTIER_PWM 9
#define PELTIER_DIR 4

//define hardware and shields used
#define USE_SENSIRION 1
#define USE_RADIO 1
#define USE_TEC

// we do not use SD on regular UNOs because a) we don't need it and b) it takes too much memory
//#define USE_SD 1 

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

// http://www.pjrc.com/teensy/td_libs_Time.html
// http://playground.arduino.cc/Code/Time
#include <Time.h> 
#include <Wire.h>
// for RTC see http://www.hobbyist.co.nz/?q=real_time_clock 
// libraries from https://github.com/PaulStoffregen/DS1307RTC and https://github.com/PaulStoffregen/Time
#include <DS1307RTC.h> 

// External descriptor of data structure
#include "MyTypes.h"

//Initialising objects and variables

unsigned long counter = 0;
time_t prev_time = 0;

packageStruct dataPackage = {myID, 0, counter, REPORT, 0, 0, 0, 0, 0};

configuration cfg = {
	540, // lights on minute ( hh * 60 + mm ) 09:00
	1260, // lights off minute				  21:00

	100, // cfg.max_light
	1, // cfg.dd_mode  0 = DD,  1 = LD, 2 = LL, 3 = DL
	
	true, // cfg.send_report
	1, // cfg.report_delay in minutes

	25.0, // cfg.set_temp
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
#define MESH_RENEWAL_TIMEOUT 10000 // changing timeout from 60.000 ms to 15.000ms
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
  if (myID >=0)
	mesh.setNodeID(myID);
  
  Serial.print(F("Starting NRF24L01: "));
  if ( mesh.begin(MESH_DEFAULT_CHANNEL, RF24_1MBPS, MESH_RENEWAL_TIMEOUT) ) 
	Serial.println(F("Done"));
  else
	Serial.println("Error");
#endif
  
  //saveConfiguration();
  setSyncProvider(RTC.get);  // get the time from the RTC
  
  // Retrieves Lights ON/OFF timer values and last light status
  //saveConfiguration();
  loadConfiguration();
  debug();
  
#if defined(USE_TEC)
  // Initialize the PWM and DIR pins as digital outputs.
  pinMode(PELTIER_PWM, OUTPUT);
  pinMode(PELTIER_DIR, OUTPUT);
#endif
}

void loop()
{
  
  //Makes sure the light is in the right status
  int minute_of_the_day = hour() * 60 + minute();
  
  switch(cfg.dd_mode) {
	  
	  case 0: //DD
	    if ( CURRENT_LIGHT > 0 ) { LightsOFF(); }
		break;
	  case 1: //LD
		if (CURRENT_LIGHT == 0 && minute_of_the_day >= cfg.lights_on && minute_of_the_day < cfg.lights_off) { LightsON(); }
		if (CURRENT_LIGHT > 0 && minute_of_the_day >= cfg.lights_off) { LightsOFF(); }
		break;
	  case 2: //LL
		if ( CURRENT_LIGHT == 0 ) { LightsON(); }
		break;
	  case 3: //DL
		if (CURRENT_LIGHT > 0 && minute_of_the_day >= cfg.lights_off && minute_of_the_day < cfg.lights_on) { LightsOFF(); }
		if (CURRENT_LIGHT == 0 && minute_of_the_day >= cfg.lights_on) { LightsON(); }
		break;
	  default:
		// do nothing
		break;
	}

#if defined(USE_RADIO)
  mesh.update();
#endif
    
  if ( (millis() - prev_time) > DELTA )  {
	prev_time = millis();

#if defined(USE_TEC)
    float delta_T = getTemperature() - cfg.set_temp;
    int power = ( abs(delta_T) / 4.0 ) * 100.0;
    abs(power) > 100 ? power = 100 : power = abs(power);
    setPeltier(power, (abs(delta_T)/delta_T ) );
#endif

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
					setInterval (rcvdPackage.set_light);
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

} // end of loop

void debug(){
  // This works only if the client is actually connected to a computer via the USB
  Serial.print("nodeID: "); Serial.println(myID);
  Serial.print("Time: "); Serial.println(now());
  Serial.print("Lights ON: "); Serial.println(cfg.lights_on); 
  Serial.print("Lights OFF: "); Serial.println(cfg.lights_off); 
  Serial.print("Report Interval: "); Serial.println(cfg.report_delay);
  Serial.print("Send report: "); Serial.println(cfg.send_report);
  Serial.print("Temperature: "); Serial.print(getTemperature());
  Serial.print("/"); Serial.println(cfg.set_temp);
  Serial.print("Humidity: "); Serial.print(getHumidity());
  Serial.print("/"); Serial.println(cfg.set_hum);
  Serial.print("Light: "); Serial.print(CURRENT_LIGHT);
  Serial.print("/"); Serial.println(cfg.max_light);
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
  if (!EEPROM.read(0)) { saveConfiguration(); } 
  EEPROM.get(1, cfg);
}

void saveConfiguration()
{
//saves values to EEPROM
  Serial.println("Saving configuration to memory");
  EEPROM.write(0, true);
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

void setInterval(byte interval)
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
    analogWrite(PELTIER_PWM, (int)(PWM / 100. * 255));
    if (direction > 0) { digitalWrite(PELTIER_DIR, HIGH ); }
    if (direction < 0) { digitalWrite(PELTIER_DIR, LOW ); }
    
    if (direction == 0) { 
        digitalWrite(PELTIER_DIR, LOW ); 
        analogWrite(PELTIER_PWM, 0);
        }
}

environment getEnvValues() {
	
	environment current_values = {0, 0, 0};
	
	// if we asked recently, we return the last values
	if ( millis() - last_sensor_reading < MIN_SENSOR_PAUSE )
		return env;
	
	last_sensor_reading = millis();
	
#if defined(USE_DHT)
	current_values.temperature =  dht.getTemperature();
	current_values.humidity =  dht.getHumidity();
#endif
#if defined(USE_SHT1x)
	current_values.temperature = sht1x.readTemperatureC();
	current_values.humidity = sht1x.readHumidity();
#endif
#if defined(USE_SENSIRION)
	uint16_t rawData;
	sht.measTemp(&rawData);                // sht.meas(TEMP, &rawData, BLOCK)
	current_values.temperature = sht.calcTemp(rawData);
	sht.measHumi(&rawData);                // sht.meas(HUMI, &rawData, BLOCK)
	current_values.humidity = sht.calcHumi(rawData, current_values.temperature);
#endif
	current_values.light = analogRead(optoresistor_PIN);

	return current_values;
}


float getTemperature()
{
	environment v = getEnvValues();
	return v.temperature;
}

float getHumidity()
{
	environment v = getEnvValues();
	return v.humidity;
}

void sendDataPackage(char header_type){

    // collect all the data in one package 
    dataPackage.orig_nodeID = myID;
    dataPackage.dest_nodeID = masterID;
    dataPackage.counter = counter++;
    dataPackage.cmd = header_type; //R Report, E event, C command
    dataPackage.current_time = now();
    
    env = getEnvValues();
    dataPackage.temp = env.temperature;
    dataPackage.hum = env.humidity;
    dataPackage.light = env.light; 

    // set data
    dataPackage.set_temp = cfg.set_temp;
    dataPackage.set_hum = cfg.set_hum; // not yet implemented
    dataPackage.set_light = CURRENT_LIGHT;
    
    //light timer data
    dataPackage.lights_on = cfg.lights_on * 60.0; // in seconds
    dataPackage.lights_off = cfg.lights_off * 60.0; // in seconds
    dataPackage.dd_mode = cfg.dd_mode;

	debug();

#if defined(USE_RADIO)
    Serial.print(F("RF: "));
     if (!mesh.write( &dataPackage, 'R', sizeof(dataPackage), masterID )){
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
