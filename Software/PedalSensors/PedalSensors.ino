#include "src/libraries/BajaCAN.h"  // https://arduino.github.io/arduino-cli/0.35/sketch-specification/#src-subfolder

#define GAS_PEDAL_ANALOG_MIN 165
#define GAS_PEDAL_ANALOG_MAX 2845

#define BRAKE_PEDAL_ANALOG_MIN 170
#define BRAKE_PEDAL_ANALOG_MAX 2675

#define GAS_PIN 34
#define BRAKE_PIN 33

int gasPedalAnalogValue = 0;
int brakePedalAnalogValue = 0;

void setup() {

  // Initialize CAN Functionality
  //setupCAN(PEDALS);

  // Open a Serial port for debugging
  Serial.begin(115200);

  pinMode(GAS_PIN, INPUT);
  pinMode(BRAKE_PIN, INPUT);
}

void loop() {


  gasPedalAnalogValue = analogRead(GAS_PIN);

  //The code below just clips the input voltage to the working range so we do not get values above 100% or below 0%
  if (gasPedalAnalogValue < GAS_PEDAL_ANALOG_MIN) gasPedalAnalogValue = GAS_PEDAL_ANALOG_MIN;
  if (gasPedalAnalogValue > GAS_PEDAL_ANALOG_MAX) gasPedalAnalogValue = GAS_PEDAL_ANALOG_MAX;

  gasPedalPercentage = floatMap(gasPedalAnalogValue, GAS_PEDAL_ANALOG_MIN, GAS_PEDAL_ANALOG_MAX, 0, 100);
  Serial.print("Gas_Analog:");
  Serial.print(gasPedalAnalogValue);
  Serial.print(", ");
  Serial.print("Gas_Percentage:");
  Serial.print(gasPedalPercentage);
  Serial.print(", ");

  brakePedalAnalogValue = analogRead(BRAKE_PIN);

  //The code below just clips the input voltage to the working range so we do not get values above 100% or below 0%
  if (brakePedalAnalogValue < BRAKE_PEDAL_ANALOG_MIN) brakePedalAnalogValue = BRAKE_PEDAL_ANALOG_MIN;
  if (brakePedalAnalogValue > BRAKE_PEDAL_ANALOG_MAX) brakePedalAnalogValue = BRAKE_PEDAL_ANALOG_MAX;

  brakePedalPercentage = floatMap(brakePedalAnalogValue, BRAKE_PEDAL_ANALOG_MIN, BRAKE_PEDAL_ANALOG_MAX, 0,  100);
  Serial.print("Brake_Analog:");
  Serial.print(brakePedalAnalogValue);
  Serial.print(", ");
  Serial.print("Brake_Percentage:");
  Serial.println(brakePedalPercentage);
  
  // Short delay for stability;
  delay(2);
}


// Function to use map with floats
float floatMap(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
