/*
Version Notes:
Version 3.0
  MAJOR:
    - calling MeasureTemp() now depends on a minimum hall count (HallThreshold). ==> TempSum and TempAvg now better reflect water actually used, rather than being subject to temp change while idle
    - MeasureFlow() now contains an offset counter to better reflect actual shower time --> when Hall count < HallThreshold, then TimeNoflow is increased by the measuring interval.
  MINOR:
    - the CheckSensor block now resides in setup(), where it belongs.
    - better re-zeroing opf variables in setup()
Version 3.1
  MINOR:
    - introduced if-loop to make sure temp measurements are not skewed due to bad contact
*/

#include <LiquidCrystal.h>              //includes library to use LCD display

//Software Version
String Version = "S4F_v3.1  ";
String Author = "Oliver Zinzen";

//Pin definitions
  const int ThermistorPin = 4;    //defines the thermistor pin (voltage devider) as a constant and an integer (Pin number)
  const int HallPin = 2;          //defines the thermistor pin (voltage devider) as a constant and an integer (Pin number)
  const int ButtonPin = 3;        //defines the button pin as a constant and an integer (Pin number)
  LiquidCrystal lcd(14, 15, 16, 17, 7, 4);  //defines LCD display communicationn pins

//Variables for input
  int ButtonState = 0;          //defines the initial ButtonState variable as 0

//Time values  (Note: if stored as unsigned 4-byte long values, overflow limit is ~50 days)
  unsigned int Std = 1000;            //sets a standard variable for time delays 
  unsigned long TimeStart = 0;        //sets a variable to store a time value (here: StartTime) as a long (4bytes) unsigned integer
  unsigned long TimeStop = 0;         //sets a variable to store a time value (here: Stop time) as a long (4bytes) unsigned integer
  unsigned long TimeNow = 0;          //sets a variable to store a time value (here: Current time) as a long (4bytes) unsigned integer
  unsigned long TimeNoflow = 0;       //sets a variable to store a time value (here: Time with no  water flow) as a long (4bytes) unsigned integer
  unsigned long Duration = 0;         //sets a variable to store a time value (here: Duration) as a long (4bytes) unsigned integer
  unsigned long DurationFlow = 0;     //sets a variable to store a time value (here: DurationFlow) as a long (4bytes) unsigned integer
  unsigned long LastMeasureInfo = 0;  //sets a variable to store the last measurement time
  unsigned long LastMeasureFlow = 0;  //sets a variable to store the last measurement time
  unsigned long LastMeasureTemp = 0;  //sets a variable to store the last measurement time
  unsigned int Interval = 1000;       //sets measurement interval
  unsigned int IntervalInfo;          //sets measurement interval
  unsigned int IntervalFlow;          //sets measurement interval
  unsigned int IntervalTemp;          //sets measurement interval
  unsigned long ExitTimer;            //sets variable to unsigned long value
 
//Thermistor variables
  int ADCout;           //defines ADCout value as an integer (this is 10-bit, 0-1023)
  float Vref = 5.0;     //defines the reference voltage (Vref, aka Vin) for the ADC, in Arduino Uno this is 5V (defined as float, b/c calculations involving it need to result in floats)
  float Vout;           //defines the voltage devider voltage as a decimal
  float R1 = 10000.0;   //defines the value of the static Resistor in the voltage devider (in Ohm) as a decimal
  float R2;             //defines the variable R2 (for Thermistor reistance) as a decimal
  float lnR2;           //defines the variable lnR2 (natural log of R2) as a decimal
  const float A = 1.105646556e-3;   //defines the thermistor variable c1 in the Steinhart-Hart equation as a float of constant value
  const float B = 2.369707092e-4;   //defines the thermistor variable c2 in the Steinhart-Hart equation as a float of constant value
  const float C = 8.420273017e-8;   //defines the thermistor variable c3 in the Steinhart-Hart equation as a float of constant value
  float Temp_C = 0.0;               //defines the variable as a decimal
  float Temp_K, TempAvg;            //defines the variables for as a decimal
  float TempSum = 0.0;              //defines the varable T_C_sum to keep a running sum of measured temperatures, 
  unsigned int N_Temp = 0;          //defines the varable to count measurement iterations

//Hall Effect Sensor variables
  unsigned int HallCount = 0;       //defines the variable for Hall Effect Sensor Counts as an unsigned integer
  float CPM = 0.0;                  //defines Counts per Minute as an unsigned integer
  const float CPL = 1141.25;        //defines the constant to convert counts to liters
  float FlowRate = 0.0;             //defines the FlowRate variable (in L/min) as a float 
  float FlowRateAvg;                //defines the average FlowRate as a float
  float FlowRateSum = 0.0;          //defines the varable as a float to keep a running sum of measured Flow Rates
  unsigned int N_FlowRate = 0;      //defines the varable to count measurement iterations
  float Volume = 0.0;               //defines the variable for water consumption (in L)

//other variables
  bool SensorCheck = false;
  int TempErrorCount = 0;                   //sets a variable to count potential faulty temp measurements, as a loose sensor connection would result in a calculated temp of absolute Zero
  unsigned int HallThreshold = 50;          //sets the threshold for Hall impulses accumulated in interval to be considered flow
  const float TempBase = 18.0;              //temperature of unheated water as base for energy calculation (in °C)
  const float c_Water_kWh = 1.1622e-3;      //specific heat os water in [kWh/(kg*°C)] -- it is 4184 J/(kg*C) and 3.6E6 J/kWh
  float Energy = 0;                         //Variable defined as float
  const float EnergyPrice = 0.28;           //price per kWh in Bernau-Barnim assuming yearlu usage of 5000kWh is approx.
                                              //0.13 Euro / kWh for Gas (Stadtwerke Bernau)
                                              //0.37 Euro / kWh for Electricity (Stadtwerke Bernau)
                                              //0.18 Euro / kWh for Gas (E.On = Grundversorger)
                                              //0.41 Euro / kWh for Electricity (E.On = Grundversorger)
                                              //--> use 0.28 Euro/kWh as a reasonable approximation for cost
  const float WaterPrice = 0.00586;         //price for water in Panketal is 2.78 Euro/m^3 + 3.08 Euro/m^3 drain water = 5.86 Euro/m^3
                                              //1m^3 = 1000L --> 0.00586 Euro/L
  float Cost; 

//==================================================
//==================================================
//==================================================

void setup() {

  Serial.begin(115200);             //begin serial communication at 115200 bits/sec
  pinMode(ButtonPin, INPUT);        //sets the ButtonPin to input mode
  pinMode(HallPin, INPUT_PULLUP);   //sets the ButtonPin to input mode
  attachInterrupt(digitalPinToInterrupt(HallPin), CountHallPulses, CHANGE); //designates the HallPin (2) to call the count() function if status change is detected
  
  lcd.begin(16, 2);                 //initializes LCD as a 16-column, 2-row display

  //The following lines display Welcome & Info message 
    lcd.clear();
    lcd.print("Showers 4 Future");
    lcd.setCursor(0, 1);
    lcd.print(" Jugend Forscht ");
    delay(Std);
    lcd.setCursor(0, 1);
    lcd.print("by ");
    lcd.print(Author);
    delay(Std);
    lcd.setCursor(0, 1);
    lcd.print("Ver.:  ");
    lcd.print(Version);
    delay(Std);

  //check if sensors are connected
  SensorCheck = true;
  MeasureTemp();
  if (Temp_C < -20) {
    Serial.println("SENSOR WARNING !!!");
    lcd.clear();
    lcd.print("Sensor not found");
    lcd.setCursor(0, 1);
    lcd.print("check connection");
    while (Temp_C <= -20) {
      delay(2000);
      if (digitalRead(ButtonPin) == HIGH) {
        
        Serial.println("Sensor Check has been skipped.");
        lcd.clear();
        lcd.print("Skipping");
        lcd.setCursor(0, 1);
        lcd.print("Sensor Check");
        delay(2000);
        break;
      }
      MeasureTemp();
    }
  }

  //re-set basic variables
    ButtonState = 0;
    TimeStart = 0;
    TimeStop = 0;
    TimeNoflow = 0;
    Temp_C = 0.0;
    TempSum = 0.0;
    TempAvg = 0.0;
    N_Temp = 0;
    SensorCheck = false;
    TempErrorCount = 0; 
    HallCount = 0; 
    FlowRate = 0.0;
    FlowRateSum = 0.0;
    N_FlowRate = 0;
    Volume = 0;
    Energy = 0;

  //Wait for start command
    lcd.clear();
    lcd.print("Press button to");
    lcd.setCursor(0, 1);
    lcd.print("measure...");

    Serial.println("\n" "Waiting for Button to be pressed...");
  
    while (digitalRead(ButtonPin) == LOW) {        //empty while loop to wait for button press to exit
      //empty
    }
    lcd.clear();
    lcd.print("OK! ...beginning");
    lcd.setCursor(0, 1);
    lcd.print("(press to stop)");
    delay(Std);
    lcd.clear();                  //clears the display
  
  //sets the reference time of "Button Push" as time since boot
    TimeStart = millis();         //sets the base time
    Serial.println();
    Serial.print("Ref. Start time since boot: 'TimeStart' = ");
    Serial.println(TimeStart);
}
//==================================================
void loop() {
  TimeNow = millis();
  IntervalInfo = 2 * Interval;    //sets measurement interval for information report (in milliseconds)
  IntervalFlow = 6 * Interval;    //sets measurement interval for FLOW measurement (in milliseconds)
  IntervalTemp = 2 * Interval;    //sets measurement interval for TEMP measurement (in milliseconds)

  //if the condition is met, the program beginns exiting
  if (digitalRead(ButtonPin) == HIGH) {
    TimeStop = millis();                  //records stoppage time
    Duration = TimeStop - TimeStart;      //calculates duration in milliseconds
    if (TimeNoflow > Duration) {
      TimeNoflow = Duration;
    }
    DurationFlow = Duration - TimeNoflow; //adjusts the Duration variable for time with flow
   
    Volume = (float(Duration)/(1000.0*60.0)) * FlowRateAvg;     //calculates consumed water volume (in L)
    Energy = Volume * c_Water_kWh * (TempAvg - TempBase);       //calculates energy (in kWh) needed to consumed heat water
    Energy = Energy / 0.7;                                      //Adjusts Energy use assuming a Water heater efficiency of 70%
    Cost = (Energy * EnergyPrice) + (Volume * WaterPrice);      //calculates approx price based on Energy + Water


    //Seial.print stoppage info
      Serial.print("'TimeStop' = ");
      Serial.println(TimeStop); 
      Serial.println();

      Serial.print("Duration since measuement start (sec)\t");
      Serial.println(Duration / 1000);
      Serial.print("Duration with flow (sec)\t");
      Serial.println(DurationFlow / 1000);
    //Seial.print results info
      Serial.print("Volume (L)\t");
      Serial.println(Volume);

      Serial.print("AvgTemp (°C)\t");
      Serial.println(TempAvg);
      
      Serial.print("Energy (Wh)\t");
      Serial.println(Energy*1000);
      
      Serial.print("Cost (Euro)\t");
      Serial.println(Cost);
    //LCD - Stoppage info
      lcd.clear();
      lcd.print("Program has been.");
      lcd.setCursor(0, 1);
      lcd.print("stopped !");
      delay(Std);

    //LCD - Results will follow and loop
      lcd.clear();
      lcd.print("Results Summary:");
      lcd.setCursor(0, 1);
      lcd.print("(info will loop)");
      delay(Std);

    //The following is the results info loop
    while (ButtonState == 0 ) {
      //LCD - Duration Info
        lcd.clear();
        lcd.print("Shower time:");
        lcd.setCursor(1, 1);
        lcd.print(float(DurationFlow)/(1000.0*60.0));
        lcd.print(" min");
        delay(Std*3);
      //LCD - Volume Info
        lcd.clear();
        lcd.print("Water Volume:");
        lcd.setCursor(1, 1);
        lcd.print(Volume);
        lcd.print(" L");
        delay(Std*3);
      //LCD - Temp Info
        lcd.clear();
        lcd.print("Average Temp.:");
        lcd.setCursor(1, 1);
        lcd.print(TempAvg);
        lcd.print(" ");
        lcd.print((char)223);
        lcd.print("C");
        delay(Std*3);
      //LCD - Energy Info
        lcd.clear();
        lcd.print("Approx. Energy:");
        lcd.setCursor(1, 1);
        lcd.print(Energy*1000);
        lcd.print(" Wh");
        delay(Std*3);
      //LCD - Cost Info
        lcd.clear();
        lcd.print("Approx. Cost:");
        lcd.setCursor(1, 1);
        lcd.print(Cost);
        lcd.print(" Euro");
        delay(Std*3);
      //LCD - Exit Option
        lcd.clear();
        lcd.print("Press Button now");
        lcd.setCursor(1, 1);
        lcd.print("to exit Loop.");
      //Exit timer
        ExitTimer = millis();
        while (millis()-ExitTimer <= 3*Std) {
          if (digitalRead(ButtonPin) == HIGH) {
            ButtonState = 1;
          }
        }
    }
    ButtonState = 0;          //resets ButtonState after exiting while loop
    lcd.clear();
    lcd.print("Restarting...");
    lcd.setCursor(0, 1);
    lcd.print("turn off anytime");
    delay(Std*2);
    setup();                  //exits while-loop when button press causes ButtoPin pull-up to HIGH
  }
  
  //to take measurements at defined intervals:
  
  if (TimeNow - LastMeasureFlow >= IntervalFlow) {
    MeasureFlow();
  }

  if (HallCount > HallThreshold) {                      //to measure Temp and update TempAvg, this requires that there is flow (to avoid average temp decreasing artificially) 
    if (TimeNow - LastMeasureTemp >= IntervalTemp) {
      MeasureTemp();
      }
  }

  if (TimeNow - LastMeasureInfo >= IntervalInfo) {
  ReportInfo();
  }
 
}
//==================================================
void CountHallPulses() {
  HallCount++;
}

//==================================================
void ReportInfo() {
  
  //Report on Flow
  Serial.print(N_FlowRate);
  Serial.print("\tFlow =\t");
  Serial.print(FlowRate);
  Serial.print("\tL/min\t\t(-> Avg Flow\t");
  Serial.print(FlowRateAvg);
  Serial.println("\tL/min)");  

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Flow ");
  lcd.print(FlowRate);
  lcd.setCursor(11, 0);
  lcd.print("L/min");

  //Report on Temp
  Serial.print(N_Temp);
  Serial.print("\tTemp = \t");
  Serial.print(Temp_C);
  Serial.print("\t°C\t\t(-> Avg Temp\t");
  Serial.print(TempAvg);
  Serial.println("\t°C)\n");
  
  lcd.setCursor(0, 1);
  lcd.print("Temp ");
  lcd.print(Temp_C);
  lcd.setCursor(11, 1);
  lcd.print((char)223);
  lcd.print("C");

  LastMeasureInfo = millis();

}

//==================================================
void MeasureFlow() {
  CPM = ((HallCount) / (IntervalFlow/1000)) * 60;   //converts counts to counts per minute
  FlowRate = CPM / CPL;                             //calculates current FlowRate in L/min
  N_FlowRate++;
  if (HallCount < HallThreshold) {                  //runs only if waterflow is close to nothing for the vast majority of the interval
    TimeNoflow = TimeNoflow + IntervalFlow;         //this increases an Shower time offset: time without flow should not be counted as shower time
  }
  FlowRateSum = FlowRateSum + FlowRate;             //keeps a running sum of flow rate measurements
  FlowRateAvg = FlowRateSum / N_FlowRate;           //updates the average FlowRate measured since start
  HallCount = 0;                                    //Resets the HallCount variable

  LastMeasureFlow = millis();
}

//==================================================
void MeasureTemp() {
  ADCout = analogRead(ThermistorPin);     //ADCout variabler defined as ADC value read at the thermistor (Arduino ADCs are 10 bit; 2^10 = 1024)
  Vout = (float)ADCout / 1023.0 * Vref;   //calculates the Vout voltage and defines it as the variable Vout
  R2 = R1 / ((Vref / Vout) - 1.0);        //R2 calculated via the voltage devider equation solved for R2
  lnR2 = log(R2);                         //defines the variable lnR2 as the natural log (base e) transformed value of the variable R2
  Temp_K = (1.0 / (A + B * lnR2 + C * lnR2 * lnR2 * lnR2));  //Steinhart-Hart equation. Calculates the temperature (in K) based on the log-transformed calculated R2 and the thermistor constants A, B & C
 
if ((Temp_K - 273.15 <= -20) && (SensorCheck == false)) {
  Temp_C = Temp_C;
  TempErrorCount++;
  if (TempErrorCount >= 5) {
    lcd.clear();
    lcd.print("Check Sensor Con.");
    lcd.setCursor(0, 1);
    lcd.print("Push to Restart!");
    if (digitalRead(ButtonPin) == HIGH) {
      setup();
    }
  }
} else {
  Temp_C = Temp_K - 273.15;               //converts Kelvin (T_K) to °C (T_C)
}
  N_Temp++;                               //keeps a running tally of measurement iterations, incrementing by +1
  TempSum = TempSum + Temp_C;             //keeps a running sum of temperature measurements
  TempAvg = TempSum / N_Temp;             //updates the average temperature measured since start 
  LastMeasureTemp = millis();
}