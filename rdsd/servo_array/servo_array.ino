#include <Servo.h> 
#include <SerialCommand.h> //https://github.com/kroimon/Wiring-SerialCommand

Servo myservo;  // create servo object to control a servo 
                // a maximum of 6 servo objects can be created 

SerialCommand sCmd;     // The SerialCommand object

//Board: http://www.emartee.com/product/42016/
//Connections: http://letsmakerobots.com/node/25923

//plug monitor 1 / channel 1 to A9 (D63) and go down skipping 44,45,46,2,3,5,6,7,8,11,12

const int servo[4][12] =
{
 {63,62,61,60,59,58,57,56,55,54,53,52}, // Monitor 1
 {51,50,49,48,47,43,42,41,40,39,38,37}, // Monitor 2
 {36,35,34,33,32,31,30,29,28,27,26,25}, // Monitor 3
 {24,23,22,21,20,19,18,17,16,15,14,13}  // Monitor 4
};

int shake = 2; // number of times the servo rotates
int pause = 1000; //pause between multiple motors rotating in ms
int rotation_delay = 300; // pause between each motor turn
boolean use_servo = 1;

void setup() 
{ 
 Serial.begin(57600);
 
 // Setup callbacks for SerialCommand commands
 sCmd.addCommand("HELP", printHelp);        // 
 sCmd.addCommand("M",     chooseChannel); // Takes two arguments. Move servo associated to monitor channel
 sCmd.addCommand("S",     changeShake); // Takes one argument. Change the shake pattern; default 2
 sCmd.addCommand("P",     changePause); // Takes one argument. Change the pause delay; default 1000
 sCmd.addCommand("L",     listValues);
 sCmd.addCommand("T",     testAll);
 sCmd.setDefaultHandler(printError);      // Handler for command that isn't matched  (says "What?")
 Serial.println("Ready.");
 
} 

void listValues(){
  Serial.print("Pause: ");
  Serial.println(pause);
  Serial.print("Shake: ");
  Serial.println(shake);
  Serial.print("Voltage: ");
  Serial.println(analogRead(0));
}


void changeShake(){
  char *arg;
  arg = sCmd.next();
  if (arg != NULL) {
    shake = atoi(arg);
  }
}

void changePause(){
  char *arg;
  arg = sCmd.next();
  if (arg != NULL) {
    pause = atoi(arg);
  }
}


void chooseChannel() {
  int monitor,channel;
  char *arg;

  arg = sCmd.next();
  if (arg != NULL) {
    monitor = atoi(arg);
  }

  arg = sCmd.next();
  if (arg != NULL) {
    channel = atoi(arg);
  }
  
  if ( monitor >= 1 && monitor <= 4 && channel >= 1 && channel <= 12 )
    {
      int pin = servo[monitor-1][channel-1];
      moveServo(pin);
      delay(pause);
      Serial.print("OK ");
      Serial.print("M");
      Serial.print(monitor);
      Serial.print("C");
      Serial.print(channel);
      Serial.print("P");
      Serial.println(pin);
    }
   else {
      printError("");
   }
}

void printHelp() {
  Serial.println("M xx yy    Moves Monitor xx (1-4) Channel yy (1-12)");
  Serial.println("S xx       Changes the shake number (default 2)");
  Serial.println("P xx       Changes the delay between rotations (default 1000)");
  Serial.println("L          List currently set values");
  Serial.println("T          Test all servos in sequence");
  Serial.println("HELP       Print this help message");
  Serial.println("");

}


void testAll() {
  for (int monitor = 1; monitor <= 4; monitor++) {
    for (int channel = 1; channel <= 12; channel++)
    {
      int pin = servo[monitor-1][channel-1];
      moveServo(pin);
      delay(pause);
    }
  } 
}

void moveServo(int pin)
{

  if ( use_servo )
  {
    myservo.attach(pin);
    for( int j = 0; j < shake; j++)
      {
        myservo.write(0);                // writes angle to 0
        delay(rotation_delay);
        myservo.write(180);
        delay(rotation_delay);
      }
    myservo.detach();
  }
  
}  

// This gets set as the default handler, and gets called when no other command matches.
void printError(const char *command) {
  Serial.println("ERROR Command not valid");
}

void loop()
{
  sCmd.readSerial();     // We don't do much, just process serial commands
}
