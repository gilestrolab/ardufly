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
  unsigned long unixTimeStamp;
  float temp;
  float hum;
  int light;
  int led_level;
} packageStruct;

#endif

