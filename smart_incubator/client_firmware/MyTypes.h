// Client/My_Types.h
// This external descriptor of data structure is needed in order to be able to pass the data structure as
// variable to functions. Without this external file, for some reason, it would not work.

#ifndef MyTypes_h
#define MyTypes_h

#include <WString.h>

typedef struct{
  int orig_nodeID;
  int dest_nodeID;
  unsigned long counter;
  char cmd;
  //
  float temp;
  float hum;
  int light;
  //
  float set_hum;
  float set_temp;
  int set_light;
  //
  time_t current_time;
  time_t lights_on;
  time_t lights_off;
  byte dd_mode;
} packageStruct;

typedef struct{
  int lights_on;
  int lights_off;
  
  byte max_light;
  byte dd_mode;
  
  bool send_report;
  byte report_delay;
  
  float set_temp;
  float set_hum;
} configuration;

typedef struct{
  float temperature;
  float humidity;
  int	light;
} environment;
#endif

