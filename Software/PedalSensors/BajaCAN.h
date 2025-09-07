/*********************************************************************************
*
*   BajaCAN.h  -- Version 2.2.1 - Native ESP32 CAN Driver
*
*   The goal of this BajaCAN header/driver is to enable all subsystems throughout
*   the vehicle to use the same variables, data types, and functions. That way,
*   any changes made to the CAN bus system can easily be applied to each subsystem
*   by simply updating the version of this file.

*
*     This driver serves several functions:
*

*     *** CAN Setup/Initialization ***
*
*     This CAN driver declares all of the variables that will be used in
*     CAN transmissions. That way, there is no confusion as to what data packet
*     a variable name refers to. In addition, declaring the variables in the CAN
*     header file allows for each variable to have the same data type. If one
*     subsystem were to use a float for an RPM value while another uses an integer,
*     there is a chance we could run into issues.
*
*     Before using CAN, several initialization steps must be taken. Since this must
*     happen during the setup() of the main code, there is a function named
*     setupCAN() that can be called in the setup() portion to execute all CAN setup.
*     Ideally, all subsystems will use the same pair of GPIO for CAN that do not
*     have any special functions. However, if setup does differ between subsystems,
*     setupCAN() has optional arguments for baud rate, GPIO, send frequency, etc.
*     The only argument that must be passed into setupCAN is a subsystem name
*     (e.g. DASHBOARD) which is used to determine which CAN messages should be
*     transmitted from each subsystem.
*
*     A pinout for the ESP32's we use can be found here:
*     https://lastminuteengineers.com/wp-content/uploads/iot/ESP32-Pinout.png
*
*     While most pins can technically be used for CAN, some should be avoided.
*     - GPIO0, GPIO2, GPIO5, GPIO12, and GPIO15 are strapping pins used during boot
*     - GPIO34, GPIO35, GPIO36, and GPIO39 are input-only pins
*     - GPIO5, GPIO18, GPIO19, and GPIO23 are used for SPI
*     - GPIO21 and GPIO22 are used for I2C
*     - GPIO1 and GPIO3 are used for Serial communication with a PC
*
*     Also in CAN setup, we must configure the data rate to be used. This can be
*     50Kbps, 80Kbps, 100Kbps, etc. We will generally use 500Kbps as it provides a
*     sufficiently high data rate which also being slow enough to make signal issues
*     less common. Note that newer ESP32 microcontrollers have an issue in their CAN
*     hardware that causes them to run at half of the provided data rate. Therefore,
*     most of our subsytems should be configured at 1000Kbps.
*
*
*
*     *** CAN Receiving ***
*
*     This part of the driver is responsible for parsing incoming packets, getting the
*     packet ID, and then sorting the receiving data into the proper variable. For
*     simplicity purposes, each subsystem sorts all valid data transmissions it
*     receives, even if that data packet isn't pertient to its function. Hardware
*     limitations should not be an issue, as CAN has its own dedicated core for
*     processing on each subsystem. The ESP32 also has plenty of memory to store all of
*     the data packets we use in CAN transmission.
*
*     When the program needs to work with a variable, such as updating a display or
*     saving data to an SD card, it can simply pass the variable knowing the CAN
*     driver has updated it with the newest value.
*
*
*
*     *** CAN Sending ***
*
*     This driver categorizes each variable based off of which subsystem should be
*     sending it. By passing through a subsystem name in setupCAN(), the subsybstem is
*     essentially telling the CAN driver, "Hey, I'm the CVT." Then, the driver would know
*     to only send CAN packets that the CVT should be updating with new data. Something
*     like the dashboard should never be reporting an RPM value to the rest of the vehicle
*     since it's not obtaining that data. This makes writing the main code easier, as CAN
*     data can just be sent on a fixed interval without any intervention by the main code.
*
*
*     *** Roadmap Ideas ***
*       - Implement a flag that is set when a data value is updated, and reset when
*         the main core reads that new data value. This could be practical in
*         situations where we need to be sure that the data we're using is recent.
*         There could even be a flag received from the main code that the CAN driver
*         uses to know when it has data to send out
*
*
************************************************************************************/

#include "driver/can.h"

// CAN Configuration
#define CAN_BAUD_RATE CAN_TIMING_CONFIG_500KBITS()
#define CAN_TX_GPIO GPIO_NUM_25
#define CAN_RX_GPIO GPIO_NUM_26

// Global variables
int canSendInterval = 25;
int lastCanSendTime = 0;
TaskHandle_t CAN_Task;

// Subsystem enumeration
enum Subsystem {
  CVT,
  DASHBOARD,
  DAS,
  WHEEL_SPEED,
  PEDALS,
  BASE_STATION
};

Subsystem currentSubsystem;

// CAN IDs
const int primaryRPM_ID = 0x01;
const int secondaryRPM_ID = 0x02;
const int primaryTemperature_ID = 0x03;
const int secondaryTemperature_ID = 0x04;
const int frontLeftWheelSpeed_ID = 0x0B;
const int frontRightWheelSpeed_ID = 0x0C;
const int rearLeftWheelSpeed_ID = 0x0D;
const int rearRightWheelSpeed_ID = 0x0E;
const int frontLeftWheelState_ID = 0x0F;
const int frontRightWheelState_ID = 0x10;
const int rearLeftWheelState_ID = 0x11;
const int rearRightWheelState_ID = 0x12;
const int gasPedalPercentage_ID = 0x15;
const int brakePedalPercentage_ID = 0x16;
const int frontBrakePressure_ID = 0x17;
const int rearBrakePressure_ID = 0x18;
const int frontLeftDisplacement_ID = 0x1F;
const int frontRightDisplacement_ID = 0x20;
const int rearLeftDisplacement_ID = 0x21;
const int rearRightDisplacement_ID = 0x22;
const int accelerationX_ID = 0x29;
const int accelerationY_ID = 0x2A;
const int accelerationZ_ID = 0x2B;
const int gyroscopeRoll_ID = 0x2C;
const int gyroscopePitch_ID = 0x2D;
const int gyroscopeYaw_ID = 0x2E;
const int gpsLatitude_ID = 0x2F;
const int gpsLongitude_ID = 0x30;
const int gpsTimeHour_ID = 0x31;
const int gpsTimeMinute_ID = 0x32;
const int gpsTimeSecond_ID = 0x33;
const int gpsDateMonth_ID = 0x34;
const int gpsDateDay_ID = 0x35;
const int gpsDateYear_ID = 0x36;
const int gpsAltitude_ID = 0x37;
const int gpsHeading_ID = 0x38;
const int gpsVelocity_ID = 0x39;
const int batteryPercentage_ID = 0x3A;
const int sdLoggingActive_ID = 0x42;
const int dataScreenshotFlag_ID = 0x43;

// CAN Variables
volatile int primaryRPM;
volatile int secondaryRPM;
volatile int primaryTemperature;
volatile int secondaryTemperature;
volatile float frontLeftWheelSpeed;
volatile float frontRightWheelSpeed;
volatile float rearLeftWheelSpeed;
volatile float rearRightWheelSpeed;
volatile int frontLeftWheelState;
volatile int frontRightWheelState;
volatile int rearLeftWheelState;
volatile int rearRightWheelState;
volatile int gasPedalPercentage;
volatile int brakePedalPercentage;
volatile int frontBrakePressure;
volatile int rearBrakePressure;
volatile float frontLeftDisplacement;
volatile float frontRightDisplacement;
volatile float rearLeftDisplacement;
volatile float rearRightDisplacement;
volatile float accelerationX;
volatile float accelerationY;
volatile float accelerationZ;
volatile float gyroscopeRoll;
volatile float gyroscopePitch;
volatile float gyroscopeYaw;
volatile float gpsLatitude;
volatile float gpsLongitude;
volatile int gpsTimeHour;
volatile int gpsTimeMinute;
volatile int gpsTimeSecond;
volatile int gpsDateMonth;
volatile int gpsDateDay;
volatile int gpsDateYear;
volatile int gpsAltitude;
volatile int gpsHeading;
volatile int gpsVelocity;
volatile int batteryPercentage;
volatile int sdLoggingActive;
volatile int dataScreenshotFlag;


float parseFloatFromBytes(uint8_t* data, int length) {
  if (length == 4) {
    float result;
    memcpy(&result, data, sizeof(float));
    return result;
  }
  return 0.0;
}

void intToBytes(int value, uint8_t* buffer, int& length) {
  length = 4;  // Always 4 bytes
  buffer[0] = (value >> 24) & 0xFF;
  buffer[1] = (value >> 16) & 0xFF; 
  buffer[2] = (value >> 8) & 0xFF;
  buffer[3] = value & 0xFF;
}

int parseIntFromBytes(uint8_t* data, int length) {
  if (length != 4) return 0;
  return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

void floatToBytes(float value, uint8_t* buffer, int& length) {
  length = sizeof(float);
  memcpy(buffer, &value, sizeof(float));
}

// CAN wrapper functions
bool sendCANInt(uint32_t id, int value) {
  can_message_t tx_message;
  tx_message.flags = CAN_MSG_FLAG_NONE;
  tx_message.identifier = id;
  tx_message.extd = 0;
  tx_message.rtr = 0;
  tx_message.ss = 0;
  tx_message.self = 0;
  tx_message.dlc_non_comp = 0;
  
  int length;
  intToBytes(value, tx_message.data, length);
  tx_message.data_length_code = length;
  return (can_transmit(&tx_message, pdMS_TO_TICKS(10)) == ESP_OK);
}

bool sendCANFloat(uint32_t id, float value) {
  can_message_t tx_message;
  tx_message.flags = CAN_MSG_FLAG_NONE;
  tx_message.identifier = id;
  tx_message.extd = 0;
  tx_message.rtr = 0;
  tx_message.ss = 0;
  tx_message.self = 0;
  tx_message.dlc_non_comp = 0;
  
  int length;
  floatToBytes(value, tx_message.data, length);
  tx_message.data_length_code = length;
  
  esp_err_t result = can_transmit(&tx_message, pdMS_TO_TICKS(5));
  if (result != ESP_OK && result != ESP_ERR_TIMEOUT) {
    Serial.print("CAN send failed for ID 0x");
    Serial.print(id, HEX);
    Serial.print(" Error: ");
    Serial.println(result);
  }
  return (result == ESP_OK);
}

// CAN Task function
void CAN_Task_Code(void *pvParameters) {
  Serial.print("CAN_Task running on core ");
  Serial.println(xPortGetCoreID());

  can_message_t message;

  for (;;) {
    // Receive messages
    if (can_receive(&message, pdMS_TO_TICKS(1)) == ESP_OK) {
      if (!(message.flags & CAN_MSG_FLAG_EXTD)) {
        uint32_t packetId = message.identifier;
        uint8_t* data = message.data;
        int dataLength = message.data_length_code;

        switch (packetId) {
          case primaryRPM_ID:
            primaryRPM = parseIntFromBytes(data, dataLength);
            break;
          case secondaryRPM_ID:
            secondaryRPM = parseIntFromBytes(data, dataLength);
            break;
          case primaryTemperature_ID:
            primaryTemperature = parseIntFromBytes(data, dataLength);
            break;
          case secondaryTemperature_ID:
            secondaryTemperature = parseIntFromBytes(data, dataLength);
            break;
          case frontLeftWheelSpeed_ID:
            frontLeftWheelSpeed = parseFloatFromBytes(data, dataLength);
            break;
          case frontRightWheelSpeed_ID:
            frontRightWheelSpeed = parseFloatFromBytes(data, dataLength);
            break;
          case rearLeftWheelSpeed_ID:
            rearLeftWheelSpeed = parseFloatFromBytes(data, dataLength);
            break;
          case rearRightWheelSpeed_ID:
            rearRightWheelSpeed = parseFloatFromBytes(data, dataLength);
            break;
          case frontLeftWheelState_ID:
            frontLeftWheelState = parseIntFromBytes(data, dataLength);
            break;
          case frontRightWheelState_ID:
            frontRightWheelState = parseIntFromBytes(data, dataLength);
            break;
          case rearLeftWheelState_ID:
            rearLeftWheelState = parseIntFromBytes(data, dataLength);
            break;
          case rearRightWheelState_ID:
            rearRightWheelState = parseIntFromBytes(data, dataLength);
            break;
          case gasPedalPercentage_ID:
            gasPedalPercentage = parseIntFromBytes(data, dataLength);
            break;
          case brakePedalPercentage_ID:
            brakePedalPercentage = parseIntFromBytes(data, dataLength);
            break;
          case frontBrakePressure_ID:
            frontBrakePressure = parseIntFromBytes(data, dataLength);
            break;
          case rearBrakePressure_ID:
            rearBrakePressure = parseIntFromBytes(data, dataLength);
            break;
          case frontLeftDisplacement_ID:
            frontLeftDisplacement = parseFloatFromBytes(data, dataLength);
            break;
          case frontRightDisplacement_ID:
            frontRightDisplacement = parseFloatFromBytes(data, dataLength);
            break;
          case rearLeftDisplacement_ID:
            rearLeftDisplacement = parseFloatFromBytes(data, dataLength);
            break;
          case rearRightDisplacement_ID:
            rearRightDisplacement = parseFloatFromBytes(data, dataLength);
            break;
          case accelerationX_ID:
            accelerationX = parseFloatFromBytes(data, dataLength);
            break;
          case accelerationY_ID:
            accelerationY = parseFloatFromBytes(data, dataLength);
            break;
          case accelerationZ_ID:
            accelerationZ = parseFloatFromBytes(data, dataLength);
            break;
          case gyroscopeRoll_ID:
            gyroscopeRoll = parseFloatFromBytes(data, dataLength);
            break;
          case gyroscopePitch_ID:
            gyroscopePitch = parseFloatFromBytes(data, dataLength);
            break;
          case gyroscopeYaw_ID:
            gyroscopeYaw = parseFloatFromBytes(data, dataLength);
            break;
          case gpsLatitude_ID:
            gpsLatitude = parseFloatFromBytes(data, dataLength);
            break;
          case gpsLongitude_ID:
            gpsLongitude = parseFloatFromBytes(data, dataLength);
            break;
          case gpsTimeHour_ID:
            gpsTimeHour = parseIntFromBytes(data, dataLength);
            break;
          case gpsTimeMinute_ID:
            gpsTimeMinute = parseIntFromBytes(data, dataLength);
            break;
          case gpsTimeSecond_ID:
            gpsTimeSecond = parseIntFromBytes(data, dataLength);
            break;
          case gpsDateMonth_ID:
            gpsDateMonth = parseIntFromBytes(data, dataLength);
            break;
          case gpsDateDay_ID:
            gpsDateDay = parseIntFromBytes(data, dataLength);
            break;
          case gpsDateYear_ID:
            gpsDateYear = parseIntFromBytes(data, dataLength);
            break;
          case gpsAltitude_ID:
            gpsAltitude = parseIntFromBytes(data, dataLength);
            break;
          case gpsHeading_ID:
            gpsHeading = parseIntFromBytes(data, dataLength);
            break;
          case gpsVelocity_ID:
            gpsVelocity = parseIntFromBytes(data, dataLength);
            break;
          case batteryPercentage_ID:
            batteryPercentage = parseIntFromBytes(data, dataLength);
            break;
          case sdLoggingActive_ID:
            sdLoggingActive = parseIntFromBytes(data, dataLength);
            break;
          case dataScreenshotFlag_ID:
            dataScreenshotFlag = parseIntFromBytes(data, dataLength);
            break;
          default:
            Serial.print("Unknown CAN ID: 0x");
            Serial.print(packetId, HEX);
            Serial.print(" Size: ");
            Serial.println(dataLength);
            break;
        }
      }
    }

    // Send messages at specified interval
    if ((millis() - lastCanSendTime) > canSendInterval) {
      lastCanSendTime = millis();

      switch (currentSubsystem) {
        case CVT:
          sendCANInt(primaryRPM_ID, primaryRPM);
          sendCANInt(secondaryRPM_ID, secondaryRPM);
          sendCANInt(primaryTemperature_ID, primaryTemperature);
          sendCANInt(secondaryTemperature_ID, secondaryTemperature);
          break;

        case WHEEL_SPEED:
          sendCANFloat(frontLeftWheelSpeed_ID, frontLeftWheelSpeed);
          sendCANFloat(frontRightWheelSpeed_ID, frontRightWheelSpeed);
          sendCANFloat(rearLeftWheelSpeed_ID, rearLeftWheelSpeed);
          sendCANFloat(rearRightWheelSpeed_ID, rearRightWheelSpeed);
          sendCANFloat(frontLeftDisplacement_ID, frontLeftDisplacement);
          sendCANFloat(frontRightDisplacement_ID, frontRightDisplacement);
          sendCANFloat(rearLeftDisplacement_ID, rearLeftDisplacement);
          sendCANFloat(rearRightDisplacement_ID, rearRightDisplacement);
          break;

        case PEDALS:
          sendCANInt(gasPedalPercentage_ID, gasPedalPercentage);
          sendCANInt(brakePedalPercentage_ID, brakePedalPercentage);
          sendCANInt(frontBrakePressure_ID, frontBrakePressure);
          sendCANInt(rearBrakePressure_ID, rearBrakePressure);
          break;

        case DAS:
          sendCANFloat(accelerationX_ID, accelerationX);
          sendCANFloat(accelerationY_ID, accelerationY);
          sendCANFloat(accelerationZ_ID, accelerationZ);
          sendCANFloat(gyroscopeRoll_ID, gyroscopeRoll);
          sendCANFloat(gyroscopePitch_ID, gyroscopePitch);
          sendCANFloat(gyroscopeYaw_ID, gyroscopeYaw);
          sendCANFloat(gpsLatitude_ID, gpsLatitude);
          sendCANFloat(gpsLongitude_ID, gpsLongitude);
          sendCANInt(gpsTimeHour_ID, gpsTimeHour);
          sendCANInt(gpsTimeMinute_ID, gpsTimeMinute);
          sendCANInt(gpsTimeSecond_ID, gpsTimeSecond);
          sendCANInt(gpsDateMonth_ID, gpsDateMonth);
          sendCANInt(gpsDateDay_ID, gpsDateDay);
          sendCANInt(gpsDateYear_ID, gpsDateYear);
          sendCANInt(gpsAltitude_ID, gpsAltitude);
          sendCANInt(gpsHeading_ID, gpsHeading);
          sendCANInt(gpsVelocity_ID, gpsVelocity);
          sendCANInt(batteryPercentage_ID, batteryPercentage);
          break;

        case DASHBOARD:
          sendCANInt(sdLoggingActive_ID, sdLoggingActive);
          sendCANInt(dataScreenshotFlag_ID, dataScreenshotFlag);
          break;
      }


// Delay to allow watchdog to reset on this core
vTaskDelay(1);
    }
  }
}

// Setup function
void setupCAN(Subsystem name, int sendInterval = 25, gpio_num_t rxGpio = CAN_RX_GPIO, gpio_num_t txGpio = CAN_TX_GPIO, can_timing_config_t baudRate = CAN_BAUD_RATE) {
  currentSubsystem = name;
  canSendInterval = sendInterval;

  can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(txGpio, rxGpio, CAN_MODE_NORMAL);
  can_timing_config_t t_config = baudRate;
  can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

  if (can_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    Serial.println("CAN Driver installed");
  } else {
    Serial.println("Failed to install CAN driver");
    return;
  }

  if (can_start() == ESP_OK) {
    Serial.println("CAN Driver started");
  } else {
    Serial.println("Failed to start CAN driver");
    return;
  }

  xTaskCreatePinnedToCore(
    CAN_Task_Code,
    "CAN_Task",
    10000,
    NULL,
    1,
    &CAN_Task,
    0);

  delay(500);
}