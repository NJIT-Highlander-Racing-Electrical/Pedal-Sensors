#include "src/libraries/BajaCAN.h"  // https://arduino.github.io/arduino-cli/0.35/sketch-specification/#src-subfolder

const int gasPedalAnalogMin = 165;
const int gasPedalAnalogMax = 2845;

const int frontBrakePressurePin = 33;
const int rearBrakePressurePin = 34;
const int gasPedalPin = 35;

const float frontBrakeR1 = 969;
const float frontBrakeR2 = 2180;
const float rearBrakeR1 = 980;
const float rearBrakeR2 = 2164;
const float gasPedalR1 = 980;
const float gasPedalR2 = 2180;

int frontBrakeAnalogValue = 0;
int rearBrakeAnalogValue = 0;

float frontBrakeVoltage = 0;
float rearBrakeVoltage = 0;

int gasPedalAnalogValue = 0;

void setup() {


  // Initialize CAN Functionality
  setupCAN(PEDALS);

  // Open a Serial port for debugging
  Serial.begin(460800);

  pinMode(frontBrakePressurePin, INPUT);
  pinMode(rearBrakePressurePin, INPUT);
  pinMode(gasPedalPin, INPUT);
}

void loop() {

  /*
  * For some reason, the analog reads on this system are not correct. The pin was given the following
  * voltages directly, and the following analog readings were calculated back to voltages based on
  * the voltage divider values. Here's what was found:
  *
  * GIVEN:    REPORTED:
  * 0.25      0.08
  * 0.5       0.32
  * 0.75      0.56
  * 1.00      0.81
  * 1.50      1.31
  * 2.00      1.80
  * 2.50      2.30
  * 3.00      2.79
  * 3.50      3.30
  * 4.00      3.90
  * 4.25      4.30
  * 4.50      4.76
  * 4.75      4.79
  * 5.00      4.79 
  *
  *
  * Up until 4V, this data is nearly perfectly linear. 4V corresponds to roughly 1300 PSI, and we don't expect
  * to see anything quite that high, so we can just rock with a linear transformation for these analog reads.
  * The calculated equation from the spreadsheet was Output Voltage = 0.993*In + 0.196V with an R^2 of 0.999.
  *
  */

  frontBrakeAnalogValue = analogRead(frontBrakePressurePin);
  frontBrakeVoltage = (float)frontBrakeAnalogValue / 4095.0 * 3.3;
  frontBrakeVoltage = frontBrakeVoltage * (frontBrakeR1 + frontBrakeR2) / frontBrakeR2;
  frontBrakeVoltage = 0.993*frontBrakeVoltage + 0.196; // See comment above about this linear transformation
  frontBrakePressure = floatMap(frontBrakeVoltage, 0.5, 4.5, 0, 1500);
  Serial.print("frontBrakeAnalogValue:");
  Serial.print(frontBrakeAnalogValue);
  Serial.print(", ");
  Serial.print("frontBrakeVoltage:");
  Serial.print(frontBrakeVoltage);
  Serial.print(", ");
  Serial.print("frontBrakePressure:");
  Serial.print(frontBrakePressure);
  Serial.print(", ");

  rearBrakeAnalogValue = analogRead(rearBrakePressurePin);
  rearBrakeVoltage = (float)rearBrakeAnalogValue / 4095.0 * 3.3;
  rearBrakeVoltage = rearBrakeVoltage * (rearBrakeR1 + rearBrakeR2) / rearBrakeR2;
  rearBrakeVoltage = rearBrakeVoltage * 0.993 + 0.196;  // See comment above about this linear transformation
  rearBrakePressure = floatMap(rearBrakeVoltage, 0.5, 4.5, 0, 1500);
  Serial.print("rearBrakeAnalogValue:");
  Serial.print(rearBrakeAnalogValue);
  Serial.print(", ");
  Serial.print("rearBrakeVoltage:");
  Serial.print(rearBrakeVoltage);
  Serial.print(", ");
  Serial.print("rearBrakePressure:");
  Serial.print(rearBrakePressure);
  Serial.print(", ");



  gasPedalAnalogValue = analogRead(gasPedalPin);

  //The code below just clips the input voltage to the working range so we do not get values above 100% or below 0%
  if (gasPedalAnalogValue < gasPedalAnalogMin) gasPedalAnalogValue = gasPedalAnalogMin;
  if (gasPedalAnalogValue > gasPedalAnalogMax) gasPedalAnalogValue = gasPedalAnalogMax;

  gasPedalPercentage = floatMap(gasPedalAnalogValue, gasPedalAnalogMin, gasPedalAnalogMax, 0, 100);
  Serial.print("gasPedalAnalogValue:");
  Serial.print(gasPedalAnalogValue);
  Serial.print(", ");
  Serial.print("gasPedalPercentage:");
  Serial.println(gasPedalPercentage);

  // Short delay for stability;
  delay(2);
}


// Function to use map with floats
float floatMap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
