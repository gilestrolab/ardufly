//http://www.timzaman.nl/?p=1089&lang=en
//http://www.elechouse.com/elechouse/index.php?main_page=product_info&cPath=152_160&products_id=2160

#include <Wire.h>
#include <MMA7660.h>

void setup()
{
  Serial.begin(9600);
  MMA7660.init();
}

void loop()
{
  int x,y,z;
  delay(100); // There will be new values every 100ms
  MMA7660.getValues(&x,&y,&z);
  Serial.print("x: ");
  Serial.print(x);
  Serial.print(" y: ");
  Serial.print(y);
  Serial.print(" z: ");
  Serial.println(z);

}
