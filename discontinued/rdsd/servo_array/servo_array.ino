#include <Servo.h> 
#include <SerialCommand.h> //https://github.com/kroimon/Arduino-SerialCommand

Servo myservo;  // create servo object to control a servo 
                // a maximum of 6 servo objects can be created 
Servo servoArray[32];

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
boolean use_servo = true;
boolean independent_mode = true; // set this to TRUE to use it without PC connected

int IR_LED_LEFT = 6;
int IR_LED_RIGHT = 7;
int powerLevel = 100;
char incomingByte='none';
String state = "OFF";
String pState;

void setup() 
{ 
 Serial.begin(57600);
 
 // Setup callbacks for SerialCommand commands
 sCmd.addCommand("HELP", printHelp);        // 
 sCmd.addCommand("M",     chooseChannel); // Takes two arguments. Move servo associated to monitor channel
 sCmd.addCommand("S",     changeShake); // Takes one argument. Change the shake pattern; default 2
 sCmd.addCommand("P",     changePause); // Takes one argument. Change the pause delay; default 1000
 sCmd.addCommand("L",     listValues);
 sCmd.addCommand("T",     testAll); // Rotates them all once
 sCmd.addCommand("MM",    testSingleMonitor); // Rotates them all once in Monitor xx
 sCmd.addCommand("AUTO",  autoMode); // Start autoMode (keep rotating at random intervals of xx-yy minutes)

//Added by Luis Garcia
 sCmd.addCommand("ST",    rotatesAll); //rotates all at once
 sCmd.addCommand("IR",    irLed); //Swith on (IR 1) or off (IR 0) IR leds, 

 sCmd.setDefaultHandler(printError);      // Handler for command that isn't matched  (says "What?")
 Serial.println("Ready.");

 //if (independent_mode) { autoMode(); }

 
} 

//Added by Luis Garcia
void irLed(){
  char *arg;
  int arg1;
  arg = sCmd.next();
  arg1 = atoi(arg);
  if( arg1 == 1) {
    digitalWrite(IR_LED_LEFT,HIGH);
    digitalWrite(IR_LED_RIGHT,HIGH);

    state = "ON";
    Serial.println(state);
  }  
  else if ( arg1 == 0) {
    digitalWrite(IR_LED_LEFT,LOW);
    digitalWrite(IR_LED_RIGHT,LOW);

    state = "OFF";
    Serial.println(state);
  }


  if  (state != pState){
    Serial.println(state);
    pState = state;
  }
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
  Serial.println("MM xx      Moves all in monitor xx");
  Serial.println("S xx       Changes the shake number (default 2)");
  Serial.println("P xx       Changes the delay between rotations (default 1000)");
  Serial.println("L          List currently set values");
  Serial.println("T          Test all servos in sequence");
  Serial.println("AUTO xx yy      Auto Mode - does not require a PC connected");
  Serial.println("HELP       Print this help message");
  Serial.println("");

}

void testSingleMonitor(){
  int monitor;
  char *arg;

  arg = sCmd.next();
  if (arg != NULL) {
    monitor = atoi(arg);
  }
  moveMonitor(monitor);
 
}


void moveMonitor(int monitor) {
  for (int channel = 1; channel <= 12; channel++)
  {
    int pin = servo[monitor-1][channel-1];
    moveServo(pin);
    delay(pause);
  }
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

void autoMode(){
  int min_min = 1; // default values
  int max_min = 7; // default values
  char *arg;

  arg = sCmd.next();
  if (arg != NULL) {
    min_min = atoi(arg);
  }

  arg = sCmd.next();
  if (arg != NULL) {
    max_min = atoi(arg);
  }


  Serial.println("Auto Mode started");
  while (true) {
      //moveMonitor(1);
      //moveMonitor(2);
      //moveMonitor(3);
      //moveMonitor(4);
      rotatesAll();
      Serial.print("Next rotation in ");
      int rand = random(min_min, max_min); //random number between min_min, max_min
      Serial.print(rand);
      Serial.println(" minutes");
      delay( (rand * 60000) ); 
  }
}

//Implemented by Luis Garcia @ polygolantree on 30/7/13
void rotatesAll(){
  Serial.println("Rotating all tubes now");
  long startTime= millis();
  int startInt= 21; //Number of the pin where the servo 1 is connected
  int maxServos=8;
  
  for (int j =0; j<32/maxServos; j++){
    for (int i=0; i<maxServos;i++){
      servoArray[i].attach(startInt+i);
      Serial.println("Attached servo");
    }

      for( int j = 0; j < shake; j++) {
        for (int i=0; i<maxServos; i++){
        servoArray[i].write(0);              // writes angle to 0
        }
        delay(rotation_delay);
         for (int i=0; i<maxServos; i++){
        servoArray[i].write(180);
         }
        delay(rotation_delay);

      }
       for (int i=0; i<maxServos; i++){
      servoArray[i].detach();
       }
    delay(10);
    startInt += maxServos; 
  }
  
  long finish = (millis() - startTime);
  Serial.println(finish);
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

