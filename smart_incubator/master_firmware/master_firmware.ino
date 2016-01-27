#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>
#include <EEPROM.h>

#include <DHT.h>
#include <SerialCommand.h>

// In theory, it is possible to run the master node on a ethernet shield
// See http://www.instructables.com/id/PART-1-Send-Arduino-data-to-the-Web-PHP-MySQL-D3js/

// http://www.pjrc.com/teensy/td_libs_Time.html
// http://playground.arduino.cc/Code/Time
#include <Time.h> 

// This external descriptor of data structure is needed in order to be able to pass the data structure as
// variable to functions. Without this external file, for some reason, it would not work.
#include "MyTypes.h"

//defines
#define VERSION 1.0
#define masterID 0

//Initialising objects
SerialCommand sCmd;

/**** Configure the nrf24l01 CE and CS pins ****/
RF24 radio(8, 7);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

//some vars and consts
unsigned long counter = 0;
packageStruct dataPackage = {masterID, 0, counter, '-', 0, 0, 0, 0, 0};

void setup()
{
  Serial.begin(115200);
  setupSerialCommands();

  mesh.setNodeID(masterID);
  mesh.begin();

  //Serial.println("Ready.");
  
}

void loop()
{
    mesh.update();
    // In addition, keep the 'DHCP service' running on the master node so addresses will
    // be assigned to the sensor nodes
    mesh.DHCP();

    // Check for incoming data from the sensors
    if(network.available()){
        RF24NetworkHeader header;
        network.peek(header);

        packageStruct rcvdPackage;
        switch(header.type){
          // Display the incoming millis() values from the sensor nodes
          case 'R': // regular, timed report
              network.read(header,&rcvdPackage,sizeof(rcvdPackage)); 
              transmitData(&rcvdPackage);
              break;
          case 'E': // event (e.g. lights on or off)
              network.read(header,&rcvdPackage,sizeof(rcvdPackage)); 
              transmitData(&rcvdPackage);
              break;
          default: network.read(header,0,0); Serial.println(header.type);break;
        }
    }
 sCmd.readSerial(); 
}

void setupSerialCommands() {
//Defines the available serial commands
  sCmd.addCommand("HELP",  printHelp);
  sCmd.addCommand("T",     setTime);
  sCmd.addCommand("L",     setLight);
  sCmd.addCommand("F",     setDelay);
  sCmd.addCommand("I",     getState);
  sCmd.addCommand("1",     setLightsONTimer);
  sCmd.addCommand("0",     setLightsOFFTimer);
  sCmd.addCommand("D",     demandDebug);
  sCmd.addCommand("M",     setLightMode);
  sCmd.setDefaultHandler(printError);      // Handler for command that isn't matched  (says "What?")
}

void printHelp() {
  Serial.println("HELP                  Print this help message");
  Serial.println("M ID DD / LD / LL     Set light mode to DD, LD, LL");
  Serial.println("T ID timestamp        Set time on node ID");
  Serial.println("L ID light (0-100)    Set light levels on node ID");
  Serial.println("I ID                  Interrogates node ID");
  Serial.println("F ID mm               Set interval (minutes) between reports from node ID");
  Serial.println("1 ID timestamp        Set time for Lights ON  - uses only HH:MM component");
  Serial.println("0 ID timestamp        Set time for Lights OFF - uses only HH:MM component");
  Serial.println("==================================================================================");
}

void printError(const char *command) {
  // This gets set as the default handler, and gets called when no other command matches.
  Serial.println("ERROR Command not valid");
}
void transmitData(packageStruct* rcvdPackage) {
  // this is used to transmit received packages to the serial port.
  // Each data line can be easily stored into a db
  Serial.print( rcvdPackage->orig_nodeID ); Serial.print (" ");
  Serial.print( rcvdPackage->cmd ); Serial.print (" ");
  Serial.print( rcvdPackage->counter ); Serial.print (" ");
  Serial.print( rcvdPackage->current_time ); Serial.print (" ");
  Serial.print( rcvdPackage->temp ); Serial.print (" ");
  Serial.print( rcvdPackage->hum ); Serial.print (" ");
  Serial.print( rcvdPackage->light ); Serial.print (" ");
  Serial.print( rcvdPackage->set_temp ); Serial.print (" ");
  Serial.print( rcvdPackage->set_hum ); Serial.print (" ");
  Serial.print( rcvdPackage->set_light ); Serial.print (" ");
  //Serial.print( hour(rcvdPackage->lights_on ) ); Serial.print (":"); Serial.print( minute(rcvdPackage->lights_on  ) ); Serial.print (" ");
  //Serial.print( hour(rcvdPackage->lights_off) ); Serial.print (":"); Serial.print( minute(rcvdPackage->lights_off ) ); Serial.print (" ");
  Serial.print( rcvdPackage->lights_on ); Serial.print (" ");
  Serial.print( rcvdPackage->lights_off ); Serial.print (" ");
  Serial.println( rcvdPackage->dd_mode );
}

void printPackage(packageStruct* rcvdPackage) {
  // A human readable Package descriptor
  // Useful for debug
  Serial.print("Originating Node: ");
  Serial.println(rcvdPackage->orig_nodeID);
  Serial.print("Destination Node: ");
  Serial.println(rcvdPackage->dest_nodeID);
  Serial.print("At time: ");
  Serial.print(hour(rcvdPackage->current_time)); Serial.print(":"); Serial.print(minute(rcvdPackage->current_time)); Serial.print(":"); Serial.print(second(rcvdPackage->current_time));
  Serial.print(" "); Serial.print(year(rcvdPackage->current_time)); Serial.print("-"); Serial.print(month(rcvdPackage->current_time)); Serial.print("-"); Serial.println(day(rcvdPackage->current_time));
  Serial.print("TS: ");
  Serial.println(rcvdPackage->current_time);
  Serial.print("T:");
  Serial.print(rcvdPackage->temp);
  Serial.print("\tH:");
  Serial.print(rcvdPackage->hum);
  Serial.print("\tL:");
  Serial.print(rcvdPackage->light);
  Serial.print("\tLvl:");
  Serial.println(rcvdPackage->set_light); 
  Serial.print("Count: ");
  Serial.println(rcvdPackage->counter);
  Serial.println("");
}

void setDelay(){
  // Changes the frequency of spontaneous reports from nodeID. 
  // A frequency of 0 will inactivate reports
  char *arg;
  int destID = 0;
  int interval = 0;

  arg = sCmd.next();
  if (arg != NULL) {
    destID = atoi(arg);
  }

  arg = sCmd.next();
  if (arg != NULL) {
    interval = atoi(arg);
  }

  dataPackage.orig_nodeID = masterID;
  dataPackage.dest_nodeID = destID;
  dataPackage.counter = counter++;
  dataPackage.cmd = 'F'; //R Report, T UpdateTime, L UpdateLight, S Update timer, F update interval
  dataPackage.set_light = interval;

  Serial.print(F("Now sending Package "));
     if (!mesh.write( &dataPackage, dataPackage.cmd, sizeof(dataPackage), destID )){
       Serial.println(F("failed"));
     } else { Serial.println("OK"); }
}

void setLightMode(){
  // Changes the frequency of spontaneous reports from nodeID. 
  // A frequency of 0 will inactivate reports
  char *arg;
  int destID = 0;
  int mode = 1;

  arg = sCmd.next();
  if (arg != NULL) {
    destID = atoi(arg);
  }

  arg = sCmd.next();
  if ((arg != NULL) and (strcmp (arg,"DD") == 0) ){ mode = 0; };
  if ((arg != NULL) and (strcmp (arg,"LD") == 0) ){ mode = 1; };
  if ((arg != NULL) and (strcmp (arg,"LL") == 0) ){ mode = 2; };

  dataPackage.orig_nodeID = masterID;
  dataPackage.dest_nodeID = destID;
  dataPackage.counter = counter++;
  dataPackage.cmd = 'M'; //R Report, T UpdateTime, L UpdateLight, S Update timer, F update interval
  dataPackage.set_light = mode;

  Serial.print(F("Now sending Package "));
     if (!mesh.write( &dataPackage, dataPackage.cmd, sizeof(dataPackage), destID )){
       Serial.println(F("failed"));
     } else { Serial.println("OK"); }
}


void setLightsONTimer(){
  // Changes the HH:MM for lights on - Called from serial port
  char *arg;
  int destID = 0;
  int lights_on = 0;

  arg = sCmd.next();
  if (arg != NULL) {
    destID = atoi(arg);
  }

  arg = sCmd.next();
  if (arg != NULL) {
    lights_on = atol(arg);
  }


  dataPackage.orig_nodeID = masterID;
  dataPackage.dest_nodeID = destID;
  dataPackage.counter = counter++;
  dataPackage.cmd = '1'; //R Report, T UpdateTime, L UpdateLight, S Update timer
  dataPackage.lights_on = lights_on;

  Serial.print(F("Now sending Package "));
     if (!mesh.write( &dataPackage, dataPackage.cmd, sizeof(dataPackage), destID )){
       Serial.println(F("failed"));
     } else { Serial.println("OK"); }
}

void setLightsOFFTimer(){
  // Changes the HH:MM for lights off - called from serial port
  char *arg;
  int destID = 0;
  unsigned long lights_off = 0;

  arg = sCmd.next();
  if (arg != NULL) {
    destID = atoi(arg);
  }

  arg = sCmd.next();
  if (arg != NULL) {
    lights_off = atol(arg);
  }

  dataPackage.orig_nodeID = masterID;
  dataPackage.dest_nodeID = destID;
  dataPackage.counter = counter++;
  dataPackage.cmd = '0'; //R Report, T UpdateTime, L UpdateLight, S Update timer
  dataPackage.lights_off = lights_off;

  Serial.print(F("Now sending Package "));
     if (!mesh.write( &dataPackage, dataPackage.cmd, sizeof(dataPackage), destID )){
       Serial.println(F("failed"));
     } else { Serial.println("OK"); }
}

void getState (){
  //interrogate the node, demand a package report - called from serial port
  char *arg;
  int destID = 0;

  arg = sCmd.next();
  if (arg != NULL) {
    destID = atoi(arg);
  }

  dataPackage.orig_nodeID = masterID;
  dataPackage.dest_nodeID = destID;
  dataPackage.counter = counter++;
  dataPackage.cmd = 'I'; //R Report, T UpdateTime, L UpdateLight, S Update timer

  Serial.print(F("Now sending Package "));
     if (!mesh.write( &dataPackage, dataPackage.cmd, sizeof(dataPackage), destID )){
       Serial.println(F("failed"));
     } else { Serial.println("OK"); }

}

void demandDebug (){
  //interrogate the node, demand a debug report - called from serial port
  char *arg;
  int destID = 0;

  arg = sCmd.next();
  if (arg != NULL) {
    destID = atoi(arg);
  }

  dataPackage.orig_nodeID = masterID;
  dataPackage.dest_nodeID = destID;
  dataPackage.counter = counter++;
  dataPackage.cmd = 'D'; //R Report, T UpdateTime, L UpdateLight, S Update timer

  Serial.print(F("Now sending Package "));
     if (!mesh.write( &dataPackage, dataPackage.cmd, sizeof(dataPackage), destID )){
       Serial.println(F("failed"));
     } else { Serial.println("OK"); }

}

void setTime (){
  //set time on the RTC module of the node - called from serial port
  char *arg;
  int destID = 0;
  unsigned long unixstamp = 0;

  arg = sCmd.next();
  if (arg != NULL) {
    destID = atoi(arg);
  }

  arg = sCmd.next();
  if (arg != NULL) {
    unixstamp = atol(arg);
  }

  dataPackage.orig_nodeID = masterID;
  dataPackage.dest_nodeID = destID;
  dataPackage.counter = counter++;
  dataPackage.cmd = 'T'; //R Report, T UpdateTime, L UpdateLight, S Update timer
  dataPackage.current_time = unixstamp;

  Serial.print(F("Now sending Package "));
     if (!mesh.write( &dataPackage, dataPackage.cmd, sizeof(dataPackage), destID )){
       Serial.println(F("failed"));
     } else { Serial.println("OK"); }
}

void setLight (){
  // turn the node lights at value 0-100% - called from serial port
  char *arg;
  int destID = 0;
  int light_level = 0;
  
  arg = sCmd.next();
  if (arg != NULL) {
    destID = atoi(arg);
  }

  arg = sCmd.next();
  if (arg != NULL) {
    light_level = atoi(arg);
  }

  dataPackage.orig_nodeID = masterID;
  dataPackage.dest_nodeID = destID;
  dataPackage.counter = counter++;
  dataPackage.cmd = 'L'; //R Report, T UpdateTime, L UpdateLight, S Update timer
  dataPackage.set_light = light_level;
    
  Serial.print(F("Now sending Package: "));
     if (!mesh.write( &dataPackage, dataPackage.cmd, sizeof(dataPackage), destID )){
       Serial.println(F("failed"));
     } else { Serial.println("OK"); }
}
