/* Sensor test sketch
  for more information see http://www.ladyada.net/make/logshield/lighttemp.html
  */

#define aref_voltage 3.3         // we tie 3.3V to ARef and measure it with a multimeter!

int photocellPin = 0;     // the cell and 10K pulldown are connected to a0
int photocellReading;     // the analog reading from the analog resistor divider

//TMP36 Pin Variables
int tempPin = 1;        //the analog pin the TMP36's Vout (sense) pin is connected to
                        //the resolution is 10 mV / degree centigrade with a
                        //500 mV offset to allow for negative temperatures
int tempReading;        // the analog reading from the sensor
    
void setup(void) {
  // We'll send debugging information via the Serial monitor
  Serial.begin(9600);   

  // If you want to set the aref to something other than 5v
  analogReference(EXTERNAL);
}


void loop(void) {
  photocellReading = analogRead(photocellPin);  
  
  Serial.print("Light reading = ");
  Serial.print(photocellReading);     // the raw analog reading
  
  // We'll have a few threshholds, qualitatively determined
  if (photocellReading < 10) {
    Serial.println(" - Dark");
  } else if (photocellReading < 200) {
    Serial.println(" - Dim");
  } else if (photocellReading < 500) {
    Serial.println(" - Light");
  } else if (photocellReading < 800) {
    Serial.println(" - Bright");
  } else {
    Serial.println(" - Very bright");
  }
  
  tempReading = analogRead(tempPin);  
  
  Serial.print("Temp reading = ");
  Serial.print(tempReading);     // the raw analog reading
  
  // converting that reading to voltage, which is based off the reference voltage
  float voltage = tempReading * aref_voltage / 1024; 
 
  // print out the voltage
  Serial.print(" - ");
  Serial.print(voltage); Serial.println(" volts");
 
  // now print out the temperature
  float temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
                                               //to degrees ((volatge - 500mV) times 100)
  Serial.print(temperatureC); Serial.println(" degress C");
 
  // now convert to Fahrenheight
  float temperatureF = (temperatureC * 9 / 5) + 32;
  Serial.print(temperatureF); Serial.println(" degress F");
 
  delay(1000);
}
