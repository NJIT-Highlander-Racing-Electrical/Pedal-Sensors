#include "src/libraries/BajaCAN.h"  // https://arduino.github.io/arduino-cli/0.35/sketch-specification/#src-subfolder

const int gasPedalAnalogMin = 165;
const int gasPedalAnalogMax = 2845;

const int frontBrakePressurePin = 33;
const int rearBrakePressurePin = 34;
const int gasPedalPin = 35;

const float frontBrakeR1 = 2180;
const float frontBrakeR2 = 969;
const float rearBrakeR1 = 2164;
const float rearBrakeR2 = 980;
const float gasPedalR1 = 2180;
const float gasPedalR2 = 980;

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

  frontBrakeAnalogValue = analogRead(frontBrakePressurePin);
  frontBrakeVoltage = (float)frontBrakeAnalogValue / 4095.0 * 3.3;
  frontBrakeVoltage = frontBrakeVoltage  *  (frontBrakeR1+frontBrakeR2) / frontBrakeR2;
  frontBrakePressure = floatMap(frontBrakeVoltage, 0.5, 4.5, 0, 1500);
  Serial.print("frontBrakeAnalogValue:");
  Serial.print(frontBrakeAnalogValue);
  Serial.print(", ");
  Serial.print("frontBrakePressure:");
  Serial.print(frontBrakePressure);
  Serial.print(", ");

  rearBrakeAnalogValue = analogRead(rearBrakePressurePin);
  rearBrakeVoltage = (float)rearBrakeAnalogValue / 4095.0 * 3.3;
  rearBrakeVoltage = rearBrakeVoltage  *  (rearBrakeR1+rearBrakeR2) / rearBrakeR2;
  rearBrakePressure = floatMap(rearBrakeVoltage, 0.5, 4.5, 0, 1500);
  Serial.print("rearBrakeAnalogValue:");
  Serial.print(rearBrakeAnalogValue);
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
