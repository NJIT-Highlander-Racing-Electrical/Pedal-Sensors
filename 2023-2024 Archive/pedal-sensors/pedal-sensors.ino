//EXTREMELY IMPORTANT NOTE: THE CAN FREQUENCY ON THIS IS NORMAL 500E3), BUT IT IS "DOUBLED" ON SOME OTHER BOARDS (1000E3) BECAUSE THEY ARE MESSED UP

//Uses sandeepmistry arduino-CAN library: https://github.com/sandeepmistry/arduino-CAN
#include <CAN.h>
#define TX_GPIO_NUM 26
#define RX_GPIO_NUM 27

#define GAS_PEDAL_ANALOG_MIN 1600
#define GAS_PEDAL_ANALOG_MAX 2650

#define BRAKE_PEDAL_ANALOG_MIN 0
#define BRAKE_PEDAL_ANALOG_MAX 3500

// Define constant values from car. Should be updated when these subsystems are done.
const int gasPedalSensor = 36;
const float MAX_GAS_PEDAL_PERCENTAGE = 100;
float gasPedalAnalogValue = 0;
float gasPedalPercentage = 0;

const int brakePedalSensor = 39;
const float MAX_BRAKE_PEDAL_PERCENTAGE = 100;
float brakePedalAnalogValue = 0;
float brakePedalPercentage = 0;

float floatMap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


void setup() {
  Serial.begin(115200);
  pinMode(gasPedalSensor, INPUT);
  pinMode(brakePedalSensor, INPUT);

  CAN.setPins(RX_GPIO_NUM, TX_GPIO_NUM);
  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    while (1)
      ;
  } else {
    Serial.println("CAN Initialized");
  }
}

void loop() {

  gasPedalAnalogValue = analogRead(gasPedalSensor);

  //Input voltages from the sensors range between roughly 185-3500

  //The code below just clips the input voltage to the 185-3500 range
  //This is so we dont get negative percentages or over 100%
  //This will need to be adjusted once we actually get the sensors in the car
  if ((gasPedalAnalogValue < GAS_PEDAL_ANALOG_MIN) && (gasPedalAnalogValue > 0)) gasPedalAnalogValue = GAS_PEDAL_ANALOG_MIN;
  if (gasPedalAnalogValue > GAS_PEDAL_ANALOG_MAX) gasPedalAnalogValue = GAS_PEDAL_ANALOG_MAX;

  gasPedalPercentage = floatMap(gasPedalAnalogValue, GAS_PEDAL_ANALOG_MIN, GAS_PEDAL_ANALOG_MAX, 0, MAX_GAS_PEDAL_PERCENTAGE);
  Serial.print("Gas_Analog:");
  Serial.print(gasPedalAnalogValue);
  Serial.print(", ");
  Serial.print("Gas_Percentage:");
  Serial.print(gasPedalPercentage);
  Serial.print(", ");

  brakePedalAnalogValue = analogRead(brakePedalSensor);

  //Input voltages from the sensors range between roughly 185-3500

  //The code below just clips the input voltage to the 185-3500 range
  //This is so we dont get negative percentages or over 100%
  //This will need to be adjusted once we actually get the sensors in the car
  if ((brakePedalAnalogValue < BRAKE_PEDAL_ANALOG_MIN) && (brakePedalAnalogValue > 0)) brakePedalAnalogValue = BRAKE_PEDAL_ANALOG_MIN;
  if (brakePedalAnalogValue > BRAKE_PEDAL_ANALOG_MAX) brakePedalAnalogValue = BRAKE_PEDAL_ANALOG_MAX;

  brakePedalPercentage = floatMap(brakePedalAnalogValue, BRAKE_PEDAL_ANALOG_MIN, BRAKE_PEDAL_ANALOG_MAX, 0, MAX_BRAKE_PEDAL_PERCENTAGE);
  Serial.print("Brake_Analog:");
  Serial.print(brakePedalAnalogValue);
  Serial.print(", ");
  Serial.print("Brake_Percentage:");
  Serial.println(brakePedalPercentage);

  //Do we want to add something that checks if the sensor value is zero?
  //If it is zero, we know the sensor is not working (min should be ~185)
  //We can report errors to DAQ
  canSender();


  delay(5);
}


void canSender() {
  // send packet: id is 11 bits, packet can contain up to 8 bytes of data
  // see "CAN-Bus Message ID Hierarchy" on GitHub for id reference


  CAN.beginPacket(0x29);          //sets the ID
  CAN.print(gasPedalPercentage);  //prints data to CAN Bus just like Serial.print
  CAN.endPacket();

  delay(5);

  CAN.beginPacket(0x2A);
  CAN.print(brakePedalPercentage);
  CAN.endPacket();


  //delay(50);
}
