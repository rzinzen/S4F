/*
The point of this sketch is to determine the Counts per Liter (CPL) value.
The impulse counts given by the Hall Effect Sensor should relate proportionally to the Volume of water passing through it.
*/

#include <LiquidCrystal.h>              //includes library to use LCD display

//Global variables:

const int HallPin = 2;          //defines the thermistor pin (voltage devider) as a constant and an integer (Pin number)
const int ButtonPin = 3;        //defines the button pin as a constant and an integer (Pin number)
LiquidCrystal lcd(14, 15, 16, 17, 7, 4);  //defines LCD display communicationn pins

unsigned int Std = 500;         //sets std time value to 0.5sec
unsigned int Iteration = 0;   
unsigned int HallCount;         
unsigned long TimeStart;
unsigned long TimeStop;

void setup() {
  Serial.begin(115200);             //begin serial communication at 9600 bits/sec
  Serial.print("Serial Comms established!");

  lcd.begin(16, 2);                 //initializes LCD as a 16-column, 2-row display
  lcd.print("LCD Comms ready");
  lcd.setCursor(0, 1);
  lcd.print("CPL utility prog");
  
  pinMode(ButtonPin, INPUT);        //sets the ButtonPin to input mode
  pinMode(HallPin, INPUT_PULLUP);   //sets the ButtonPin to input mode
  attachInterrupt(digitalPinToInterrupt(HallPin), CountHallPulses, CHANGE); //designates the HallPin (2) to call the count() function if status change is detected

  delay(Std*4);
}

void loop() {
    
  lcd.clear();
  lcd.print("push button to ");
  lcd.setCursor(0, 1);
  lcd.print("start counting..");

  while (digitalRead(ButtonPin) == LOW) {
 
  }
  delay(200);             //short delay to prevent button bounce from registering

  HallCount = 0;
  TimeStart = millis();
  
  lcd.clear();
  lcd.print("counting.....");    
  
  while (digitalRead(ButtonPin) == LOW) {
    //empty loop to wait for button push
  }
  delay(200);             //short delay to prevent button bounce from registering

  TimeStop = millis();
  Iteration++;

  Serial.print("\n No. ");
  Serial.println(Iteration);
  Serial.print("Time (sec):\t");
  Serial.println((TimeStop - TimeStart) / 1000.0); 
  Serial.print("Counts:\t");
  Serial.println(HallCount);

  lcd.clear();
  lcd.print(Iteration);
  lcd.print(" Time:");
  lcd.setCursor(8, 0);
  lcd.print((TimeStop - TimeStart) / 1000);
  lcd.setCursor(0, 1);
  lcd.print("Counts:");
  lcd.setCursor(8, 1);
  lcd.print(HallCount);

  delay(Std*2);

  while (digitalRead(ButtonPin) == LOW) {
    //empty loop to wait for button push
  }
  delay(100);             //short delay to prevent button bounce from registering

}

void CountHallPulses() {
  HallCount++;
}
