/*********************************************************************************
*
*   BajaCAN.h  -- Version 1.5.0
*
*   The goal of this BajaCAN header/driver is to enable all subsystems throughout
*   the vehicle to use the same variables, data types, and functions. That way,
*   any changes made to the CAN bus system can easily be applied to each subsystem
*   by simply updating the version of this file.
*
*   This driver is based on the arduino-CAN library by sandeepmistry. Since all of
*   the built-in functions to this library work well, we can simply call its
*   functions in this program.
*
*   This driver serves several functions:
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
*     By default, arduino-CAN uses GPIO_5 for CAN-TX and GPIO-4 for CAN_RX. We will
*     most likely not use these defaults as GPIO_5 is also used for SPI Chip Select.
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
*     *** Status Bits ***
*
*      Each subsystem has one integer dedicated to status bits. If all is good, the subsystem
*      transmits a 1, but any issues result in a 0 being transmitted. This allows the data systems
*      (Base Station and DAS) to get reports of subsystem health. These data systems can also send
*      a packet with the RTR (Remote Transmission Request) bit set, essentially requesting that the
*      subsystem reports its health. This is useful in the situation where a subsystem loses
*      communication after it has been established. Ideally, this can also be expanded to use all
*      16 bits for different status flags. For example, the wheel speed subsystem could use 8 flags
*      for each of the 8 sensors onboard.
*
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

// Include sandeepmistry's arduino-CAN library
#include "arduino-CAN/src/CAN.h"

// Definitions for all CAN setup parameters
#define CAN_BAUD_RATE 500E3
#define CAN_TX_GPIO 25
#define CAN_RX_GPIO 26

// Number of milliseconds to wait between transmissions
int canSendInterval = 50;
// Definition to log the last time that a CAN message was sent
int lastCanSendTime = 0;

// Task to run on second core (dual-core processing)
TaskHandle_t CAN_Task;

// Defines a list of acceptable subsystem names that can be used in setupCAN(<subsystem>)
enum Subsystem {
  CVT,
  DASHBOARD,
  DAS,
  WHEEL_SPEED,
  PEDALS,
  BASE_STATION
};

// Declare the variable for the subsystem this driver should use
// This will be used to modify the data that is sent from this
Subsystem currentSubsystem;

// Definitions for all CAN IDs (in hex form) here:

// CVT Tachometer CAN IDs
const int primaryRPM_ID = 0x01;
const int secondaryRPM_ID = 0x02;
const int primaryTemperature_ID = 0x03;
const int secondaryTemperature_ID = 0x04;

// Wheel Speed Sensors CAN IDs
const int frontLeftWheelSpeed_ID = 0x0B;
const int frontRightWheelSpeed_ID = 0x0C;
const int rearLeftWheelSpeed_ID = 0x0D;
const int rearRightWheelSpeed_ID = 0x0E;

// Wheel Speed States CAN IDs (for slip and skid)
const int frontLeftWheelState_ID = 0x0F;
const int frontRightWheelState_ID = 0x10;
const int rearLeftWheelState_ID = 0x11;
const int rearRightWheelState_ID = 0x12;

// Pedal Sensors CAN IDs
const int gasPedalPercentage_ID = 0x15;
const int brakePedalPercentage_ID = 0x16;
const int frontBrakePressure_ID = 0x17;
const int rearBrakePressure_ID = 0x18;

// Suspension Displacement CAN IDs
const int frontLeftDisplacement_ID = 0x1F;
const int frontRightDisplacement_ID = 0x20;
const int rearLeftDisplacement_ID = 0x21;
const int rearRightDisplacement_ID = 0x22;

// DAS (Data Acquisition System) CAN IDs
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

// Dashboard CAN IDs
const int sdLoggingActive_ID = 0x42;
const int dataScreenshotFlag_ID = 0x43;

// Status Bits
const int statusCVT_ID = 0x5A;
const int statusBaseStation_ID = 0x5B;
const int statusDashboard_ID = 0x5C;
const int statusDAS_ID = 0x5D;
const int statusWheels_ID = 0x5E;
const int statusPedals_ID = 0x5F;

// Declarations for all variables to be used here:

// CVT Tachometer
volatile int primaryRPM;
volatile int secondaryRPM;
volatile int primaryTemperature;
volatile int secondaryTemperature;

// Wheel Speed Sensors CAN
volatile float frontLeftWheelSpeed;
volatile float frontRightWheelSpeed;
volatile float rearLeftWheelSpeed;
volatile float rearRightWheelSpeed;

// Wheel States
volatile int frontLeftWheelState;
volatile int frontRightWheelState;
volatile int rearLeftWheelState;
volatile int rearRightWheelState;

// Pedal Sensors CAN
volatile int gasPedalPercentage;
volatile int brakePedalPercentage;
volatile int frontBrakePressure;
volatile int rearBrakePressure;

// Suspension Displacement CAN
volatile float frontLeftDisplacement;
volatile float frontRightDisplacement;
volatile float rearLeftDisplacement;
volatile float rearRightDisplacement;

// DAS (Data Acquisition System) CAN
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

// Dashboard CAN
volatile int sdLoggingActive;
volatile int dataScreenshotFlag;

// Status Bits
volatile uint8_t statusCVT;
volatile uint8_t statusBaseStation;
volatile uint8_t statusDashboard;
volatile uint8_t statusDAS;
volatile uint8_t statusWheels;
volatile uint8_t statusPedals;

// Declaraction for CAN_Task_Code second core program
void CAN_Task_Code(void *pvParameters);

// This setupCAN() function should be called in void setup() of the main program
void setupCAN(Subsystem name, int sendInterval = canSendInterval, int rxGpio = CAN_RX_GPIO, int txGpio = CAN_TX_GPIO, int baudRate = CAN_BAUD_RATE) {

  // Assign currentSubsystem based on passed through name
  currentSubsystem = name;

  // If user specified a different sendInterval in setupCAN(), assign that
  // Otherwise, it will just remain as default canSendInterval value
  canSendInterval = sendInterval;

  // Reconfigure pins used for CAN from defaults
  CAN.setPins(rxGpio, txGpio);

  if (!CAN.begin(baudRate)) {
    Serial.println("Starting CAN failed!");
    while (1)
      ;
  } else {
    Serial.println("CAN Initialized");
  }

  // create a task that will be executed in the CAN_Task_Code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
    CAN_Task_Code, /* Task function. */
    "CAN_Task",    /* name of task. */
    10000,         /* Stack size of task */
    NULL,          /* parameter of the task */
    1,             /* priority of the task */
    &CAN_Task,     /* Task handle to keep track of created task */
    0);            /* pin task to core 0 */

  // Delay for stability; may not be necessary but only executes once
  delay(500);
}

// CAN_Task executes on secondary core of ESP32 and its sole function is CAN
// All other processing is done on primary core
void CAN_Task_Code(void *pvParameters) {
  Serial.print("CAN_Task running on core ");
  Serial.println(xPortGetCoreID());

  for (;;) {

    // Check if a packet has been received
    // Returns the packet size in bytes or 0 if no packet received
    int packetSize = CAN.parsePacket();
    int packetId;

    if ((packetSize > 0) || (CAN.packetRtr() && CAN.packetId() != -1)) {
      // received a packet
      packetId = CAN.packetId();  // Get the packet ID

      // Sort data packet to correct variable based on ID
      switch (packetId) {

        // Primary RPM Case
        case primaryRPM_ID:
          primaryRPM = CAN.parseInt();
          break;

        // Secondary RPM Case
        case secondaryRPM_ID:
          secondaryRPM = CAN.parseInt();
          break;

        // CVT Primary Temperature Case
        case primaryTemperature_ID:
          primaryTemperature = CAN.parseInt();
          break;

        // CVT Secondary Temperature Case
        case secondaryTemperature_ID:
          secondaryTemperature = CAN.parseInt();
          break;

        // Wheel Speed Sensors RPM Case
        case frontLeftWheelSpeed_ID:
          frontLeftWheelSpeed = CAN.parseFloat();
          break;

        // Wheel Speed Sensors RPM Case
        case frontRightWheelSpeed_ID:
          frontRightWheelSpeed = CAN.parseFloat();
          break;

        // Wheel Speed Sensors RPM  Case
        case rearLeftWheelSpeed_ID:
          rearLeftWheelSpeed = CAN.parseFloat();
          break;

        // Wheel Speed Sensors RPM Case
        case rearRightWheelSpeed_ID:
          rearRightWheelSpeed = CAN.parseFloat();
          break;

        // Wheel Speed Sensors State Case
        case frontLeftWheelState_ID:
          frontLeftWheelState = CAN.parseInt();
          break;

        // Wheel Speed Sensors State Case
        case frontRightWheelState_ID:
          frontRightWheelState = CAN.parseInt();
          break;

        // Wheel Speed Sensors State Case
        case rearLeftWheelState_ID:
          rearLeftWheelState = CAN.parseInt();
          break;

        // Wheel Speed Sensors State Case
        case rearRightWheelState_ID:
          rearRightWheelState = CAN.parseInt();
          break;

        // Pedal Sensors Case
        case gasPedalPercentage_ID:
          gasPedalPercentage = CAN.parseInt();
          break;

        // Pedal Sensors Case
        case brakePedalPercentage_ID:
          brakePedalPercentage = CAN.parseInt();
          break;

        // Pedal Sensors Case
        case frontBrakePressure_ID:
          frontBrakePressure = CAN.parseInt();
          break;

        // Pedal Sensors Case
        case rearBrakePressure_ID:
          rearBrakePressure = CAN.parseInt();
          break;

        // Suspension Displacement Case
        case frontLeftDisplacement_ID:
          frontLeftDisplacement = CAN.parseFloat();
          break;

        // Suspension Displacement Case
        case frontRightDisplacement_ID:
          frontRightDisplacement = CAN.parseFloat();
          break;

        // Suspension Displacement Case
        case rearLeftDisplacement_ID:
          rearLeftDisplacement = CAN.parseFloat();
          break;

        // Suspension Displacement Case
        case rearRightDisplacement_ID:
          rearRightDisplacement = CAN.parseFloat();

          break;

        // DAS Accel Case
        case accelerationX_ID:
          accelerationX = CAN.parseFloat();
          break;

        // DAS Accel Case
        case accelerationY_ID:
          accelerationY = CAN.parseFloat();
          break;

        // DAS Accel Case
        case accelerationZ_ID:
          accelerationZ = CAN.parseFloat();
          break;

        // DAS Gyro Case
        case gyroscopeRoll_ID:
          gyroscopeRoll = CAN.parseFloat();
          break;

        // DAS Gyro Case
        case gyroscopePitch_ID:
          gyroscopePitch = CAN.parseFloat();
          break;

        // DAS Gyro Case
        case gyroscopeYaw_ID:
          gyroscopeYaw = CAN.parseFloat();
          break;

        // DAS GPS Position Case
        case gpsLatitude_ID:
          gpsLatitude = CAN.parseFloat();
          break;

        // DAS GPS Position Case
        case gpsLongitude_ID:
          gpsLongitude = CAN.parseFloat();
          break;

        // DAS GPS Time Case
        case gpsTimeHour_ID:
          gpsTimeHour = CAN.parseInt();
          break;

        // DAS GPS Time Case
        case gpsTimeMinute_ID:
          gpsTimeMinute = CAN.parseInt();
          break;

        // DAS GPS Time Case
        case gpsTimeSecond_ID:
          gpsTimeSecond = CAN.parseInt();
          break;

        // DAS GPS Date Case
        case gpsDateMonth_ID:
          gpsDateMonth = CAN.parseInt();
          break;

        // DAS GPS Date Case
        case gpsDateDay_ID:
          gpsDateDay = CAN.parseInt();
          break;

        // DAS GPS Date Case
        case gpsDateYear_ID:
          gpsDateYear = CAN.parseInt();
          break;

        // DAS GPS Altitude Case
        case gpsAltitude_ID:
          gpsAltitude = CAN.parseInt();
          break;

        // DAS GPS Heading Case
        case gpsHeading_ID:
          gpsHeading = CAN.parseInt();
          break;

        // DAS GPS Velocity Case
        case gpsVelocity_ID:
          gpsVelocity = CAN.parseInt();
          break;

        // DAS Battery Percentage Case
        case batteryPercentage_ID:
          batteryPercentage = CAN.parseInt();
          break;

        // Dashboard SD Logging Active Case
        case sdLoggingActive_ID:
          sdLoggingActive = CAN.parseInt();
          break;

        // Dashboard Data Screenshot Flag Case
        case dataScreenshotFlag_ID:
          dataScreenshotFlag = CAN.parseInt();
          break;

        default:
          Serial.print("Unrecognized CAN ID: ");
          Serial.println(packetId);

          while (CAN.available()) {
            int value = CAN.read();  // Read each byte individually
            Serial.print(value);
          }
          Serial.println();
          break;
      }
    }

    if ((millis() - lastCanSendTime) > canSendInterval) {

      lastCanSendTime = millis();

      delay(canSendInterval / 2);  // Delay for half of our send interval. This should allow Watchdog to reset during IDLE without interfering with the functionality of the program. For the default interval (100ms), we provide a 50ms delay

      switch (currentSubsystem) {

        case CVT:
          CAN.beginPacket(primaryRPM_ID);
          CAN.print(primaryRPM);
          CAN.endPacket();

          CAN.beginPacket(secondaryRPM_ID);
          CAN.print(secondaryRPM);
          CAN.endPacket();

          CAN.beginPacket(primaryTemperature_ID);
          CAN.print(primaryTemperature);
          CAN.endPacket();

          CAN.beginPacket(secondaryTemperature_ID);
          CAN.print(secondaryTemperature);
          CAN.endPacket();

          break;

        case WHEEL_SPEED:

          // WHEEL RPMs
          CAN.beginPacket(frontLeftWheelSpeed_ID);
          CAN.print(frontLeftWheelSpeed, 2);
          CAN.endPacket();

          CAN.beginPacket(frontRightWheelSpeed_ID);
          CAN.print(frontRightWheelSpeed, 2);
          CAN.endPacket();

          CAN.beginPacket(rearLeftWheelSpeed_ID);
          CAN.print(rearLeftWheelSpeed, 2);
          CAN.endPacket();

          CAN.beginPacket(rearRightWheelSpeed_ID);
          CAN.print(rearRightWheelSpeed, 2);
          CAN.endPacket();

          // WHEEL STATES
          CAN.beginPacket(frontLeftWheelState_ID);
          CAN.print(frontLeftWheelState);
          CAN.endPacket();

          CAN.beginPacket(frontRightWheelState_ID);
          CAN.print(frontRightWheelState);
          CAN.endPacket();

          CAN.beginPacket(rearLeftWheelState_ID);
          CAN.print(rearLeftWheelState);
          CAN.endPacket();

          CAN.beginPacket(rearRightWheelState_ID);
          CAN.print(rearRightWheelState);
          CAN.endPacket();

          // SUSPENSION DISPLACEMENTS
          CAN.beginPacket(frontLeftDisplacement_ID);
          CAN.print(frontLeftDisplacement, 2);
          CAN.endPacket();

          CAN.beginPacket(frontRightDisplacement_ID);
          CAN.print(frontRightDisplacement, 2);
          CAN.endPacket();

          CAN.beginPacket(rearLeftDisplacement_ID);
          CAN.print(rearLeftDisplacement, 2);
          CAN.endPacket();

          CAN.beginPacket(rearRightDisplacement_ID);
          CAN.print(rearRightDisplacement, 2);
          CAN.endPacket();

          break;

        case PEDALS:
          CAN.beginPacket(gasPedalPercentage_ID);
          CAN.print(gasPedalPercentage);
          CAN.endPacket();

          CAN.beginPacket(brakePedalPercentage_ID);
          CAN.print(brakePedalPercentage);
          CAN.endPacket();

          CAN.beginPacket(frontBrakePressure_ID);
          CAN.print(frontBrakePressure);
          CAN.endPacket();

          CAN.beginPacket(rearBrakePressure_ID);
          CAN.print(rearBrakePressure);
          CAN.endPacket();

          break;

        case DAS:
          CAN.beginPacket(accelerationX_ID);
          CAN.print(accelerationX, 2);
          CAN.endPacket();

          CAN.beginPacket(accelerationY_ID);
          CAN.print(accelerationY, 2);
          CAN.endPacket();

          CAN.beginPacket(accelerationZ_ID);
          CAN.print(accelerationZ, 2);
          CAN.endPacket();

          CAN.beginPacket(gyroscopeRoll_ID);
          CAN.print(gyroscopeRoll, 2);
          CAN.endPacket();

          CAN.beginPacket(gyroscopePitch_ID);
          CAN.print(gyroscopePitch, 2);
          CAN.endPacket();

          CAN.beginPacket(gyroscopeYaw_ID);
          CAN.print(gyroscopeYaw, 2);
          CAN.endPacket();

          CAN.beginPacket(gpsLatitude_ID);
          CAN.print(gpsLatitude, 6);
          CAN.endPacket();

          CAN.beginPacket(gpsLongitude_ID);
          CAN.print(gpsLongitude, 6);
          CAN.endPacket();

          CAN.beginPacket(gpsTimeHour_ID);
          CAN.print(gpsTimeHour);
          CAN.endPacket();

          CAN.beginPacket(gpsTimeMinute_ID);
          CAN.print(gpsTimeMinute);
          CAN.endPacket();

          CAN.beginPacket(gpsTimeSecond_ID);
          CAN.print(gpsTimeSecond);
          CAN.endPacket();

          CAN.beginPacket(gpsDateMonth_ID);
          CAN.print(gpsDateMonth);
          CAN.endPacket();

          CAN.beginPacket(gpsDateDay_ID);
          CAN.print(gpsDateDay);
          CAN.endPacket();

          CAN.beginPacket(gpsDateYear_ID);
          CAN.print(gpsDateYear);
          CAN.endPacket();

          CAN.beginPacket(gpsAltitude_ID);
          CAN.print(gpsAltitude);
          CAN.endPacket();

          CAN.beginPacket(gpsHeading_ID);
          CAN.print(gpsHeading);
          CAN.endPacket();

          CAN.beginPacket(gpsVelocity_ID);
          CAN.print(gpsVelocity);
          CAN.endPacket();

          CAN.beginPacket(batteryPercentage_ID);
          CAN.print(batteryPercentage);
          CAN.endPacket();
          break;

        case DASHBOARD:
          CAN.beginPacket(sdLoggingActive_ID);
          CAN.print(sdLoggingActive);
          CAN.endPacket();

          CAN.beginPacket(dataScreenshotFlag_ID);
          CAN.print(dataScreenshotFlag);
          CAN.endPacket();

          break;
         
      }
    }
  }
}
