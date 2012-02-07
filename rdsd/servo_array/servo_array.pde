#include <Servo.h> 
 
Servo myservo;  // create servo object to control a servo 
                // a maximum of 6 servo objects can be created 
const int max_servos = 9;
const int servos[] = {2,3,4,5,6,7,8,9,10}; // Servo Pins
const int shake = 1; // number of times the servo rotates

char servoNum[2];
int index;

 
void setup() 
{ 
 Serial.begin(19200);
 Serial.println("Ready to accept commands.");  
} 
 
void moveServo(int s)
{
 
  Serial.print("Moving Servo number ");
  Serial.println(s);
  
  myservo.attach(s);
  for( int j = 0; j < shake; j++)
    {
      myservo.write(0);                // writes angle to 0
      delay(400);
      myservo.write(180);
      delay(400);
    }
  myservo.detach();
}  


void loop() 
{ 
  
  while ( Serial.available() )    // Serial available
    {  
          char cmd = Serial.read();  // read Serial Input
          
          if ( index < 2 && cmd >= '0' && cmd <= '9'  )
            {
              servoNum[index++] = cmd;
            }
          if ( index >= 2)
           {
              servoNum[index] = 0;
              int c = atoi(servoNum);
              if ( c <= max_servos )
                {
                  moveServo( servos[c] );
                }
              index = 0;
            }
    }       
}

 


