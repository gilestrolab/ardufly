#include <AFMotor.h>
//Giorgio Gilestro <giorgio@gilest.ro>
//
//Code adapted from http://www.ladyada.net/make/logshield/lighttemp.html
//
//Free pins on servo shield:
//All 6 analog input pins are available. 
//They can also be used as digital pins (pins #14 thru 19)
//Digital pin 2, and 13 are not used.

#define aref_voltage 5.0         // SPECIFY VOLTAGE USED FOR SENSORS
#define steps_motor 200
#define port 2

// Connect a stepper motor with 200 steps per revolution (1.5 degree)
// to motor port #2 (M3 and M4)
const int len_sequence = 10;
AF_Stepper motor(steps_motor, port);

// Gear factor (ratio between number of teeth)
const float gear_factor = 72/12;

//Command from serial port
String command; char c;

//Photocell Variables
int photocellPin = 0;     // the cell and 10K pulldown are connected to a0
int photocellReading;     // the analog reading from the analog resistor divider

//TMP36 Pin Variables
const int tempPin = 1;  //the analog pin the TMP36's Vout (sense) pin is connected to
                        //the resolution is 10 mV / degree centigrade with a
                        //500 mV offset to allow for negative temperatures
int tempReading;        // the analog reading from the sensor

//Pushbutton to pause / resume
const int pushPin = A3;

//LED Lights, controlled to a TIP102 Transistor
const int LightPin = A2;
const int LEDPin = 13;

boolean brake = true; boolean day = false;
const String stat[] = {"OFF", "ON"};
const int rotationmode[] = {SINGLE, DOUBLE, INTERLEAVE, MICROSTEP};
int sequence[] = {180, 60, 90, 45, 180, 10, 90, 10, 180, 180}; // sequence of rotation degrees, time to wait in seconds
int rotations = 0; // all rotations done since boot

int step; 
unsigned long interval = 0; 
int count = 0;
bool startRevolution = true;
unsigned long previousTime = 0;

int readLight() {
  photocellReading = analogRead(photocellPin);
  return photocellReading;
}

void getNewSequence (String entry) {
  for (int i = 0; i < len_sequence; i = i+2 ) {
    //sequence[i] = entry.substring(i*3, i*3+3);
    //sequence[i+1] = entry.substring(i*3+3, i*3+6);
  }
}

float readTemperature() {
  tempReading = analogRead(tempPin);
  float voltage = tempReading * aref_voltage;
  
  voltage /= 1024.0;
  //Serial.print(voltage); Serial.println(" volts");
  float temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
                                               //to degrees ((volatge - 500mV) times 100)
  return temperatureC;
}

boolean isButtonPressed() {
 //check if button is pressed
 int val = digitalRead(pushPin);
 if (val == HIGH) {
     brake = !brake;
     digitalWrite(LEDPin, HIGH); 
     delay(100); 
     digitalWrite(LEDPin, LOW); // blink LED
 }
}

boolean lights(char l) {
  if (l == '0') { digitalWrite(LightPin, LOW); }
  if (l == '1') { digitalWrite(LightPin, HIGH); }
  return (l == '1');
}

void setup() { //call this once at the beginning

    pinMode(LEDPin, OUTPUT); // declare LED as output
    pinMode(LightPin, OUTPUT); // declare transistor PIN as output
    pinMode(pushPin, INPUT);  // declare pushbutton as input

    Serial.begin(9600); //start up serial port
    motor.setSpeed(20);  // 20 rpm   
    int rotation = rotationmode[0];
    //analogReference(EXTERNAL);    // If you want to set the aref to something other than 5v
}

void loop() { //main loop

 unsigned long currentTime = millis();
 isButtonPressed();

 while( Serial.available() && c!= '\n' ) {  // READS AN ENTIRE LINE FROM SERIAL
   c = Serial.read();
   command = command + c;
 }
 
 String cmd = command[0];

 if (cmd == "L") { //L1 - L0 -> LIGHTS ON OR OFF
     day = lights (command[1]);
 }
 if (cmd == "M") { //M1 - M0 -> START STOP MOTOR
     brake = (command[1] == '0');
 }
 if (cmd == "R") { //RELEASE MOTOR
    motor.release();
    brake = true;
 }
 if (cmd == "S") { //SET NEW SPINNING SEQUENCE
     getNewSequence ( command.substring(1) );
 }
 //if (cmd == "E") { //SET ROTATION MODE
   //  rotation = rotationmode[ command[1] ];
// }
 if (cmd == "A") { //READ and REPORTS AMBIENT VALUES
     Serial.println( readTemperature() );
     Serial.println( readLight() );
 }
 if (cmd == "?") { //PRINT SOME INFO ABOUT THE CURRENT STATUS
   Serial.println("##### Actual conditions ######");
   Serial.println("Lights are " + stat[day]);
   Serial.println("Motors are " + stat[!brake]);
   Serial.println("Spinning Sequence: ");// Serial.println(sequence);
   Serial.print("Spinning round: "); Serial.println( count );//( count - 2 ) / 2 + 1);
   Serial.print("Rotations so far: "); Serial.println( rotations );
   Serial.print("Next revolution in: "); Serial.println( (interval - (currentTime - previousTime) ) / 1000);
   Serial.print("Temperature reading: "); Serial.println(readTemperature());
   Serial.print("Light reading: "); Serial.println(readLight());
   
 }
 
 // MOTOR CYCLE STARTS HERE
 
 if ( !brake & startRevolution & currentTime - previousTime > interval ) {
  step = (sequence[count] / 360.0 ) * gear_factor * steps_motor; // converts degrees to steps
  interval = 1000.0 * sequence[count+1]; // convert seconds to milliseconds
  count = count + 2; if (count >= len_sequence ) { count = 0; } 

  motor.step(step, FORWARD, SINGLE); 
  previousTime = currentTime;
  startRevolution = false;
  rotations++;
 }

 if ( !brake & !startRevolution & currentTime - previousTime > interval ) { 
  motor.step(step, BACKWARD, SINGLE); 
  previousTime = currentTime;
  startRevolution = true;
  rotations++;
 }

 //TURN LED ON IF IN PAUSE
 if ( brake ) { digitalWrite(LEDPin, HIGH); }
 else { digitalWrite(LEDPin, LOW); }

 delay(100); command = "";




}





