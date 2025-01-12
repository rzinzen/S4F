# S4F
This project titled S4F ("Showers for Future") was designed and realized by Oliver R. Zinzen as a Science Fair project.
It descroibes an adapter compatible with 1/2-inch water pipes that houses a thermistor as a temperature sensor and which is to be combined with a commercially available Hall-Effect based 5V flow sensor. These sensors are meant to be connected to an Arduino Uno via a custom-solderd shield that also accomodates at 16x2 LCD display.
The following files are provided:
- Arduino Sketches
  - a main operating program called "S4F_vX.X" to read the sensors, address the LCD and monitor an input button.
  - a utility program called "S4F_CPLutility_v1.0" that helps determine the CPL (Counts per Liter) value 
- A wiring diagram for the electronics and how they are connected to the Arduino Uno
- A soldering plan for the custom shield
- STL 3D-model files so that the following items can be printed:
  - the Thermistor Housing with intrerenal 1/2-inch pipe threads
  - the enclosure that contains the electronics (Arduino Uno, electronics sield, main switch, battery, input button)
