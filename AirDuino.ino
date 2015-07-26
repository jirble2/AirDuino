#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "SPI.h"
#include <Adafruit_10DOF.h>
#include <Adafruit_BMP085_U.h>
#include <Adafruit_L3GD20_U.h>
#include <Adafruit_LSM303_U.h>
#include <Adafruit_Sensor.h>
#include <DS3232RTC.h>
#include <math.h>
#include <SD.h>
#include <Time.h>
#include <Ultrasonic.h>
#include <Wire.h>

// Pin declarations
#define STMPE_CS 8
#define TFT_DC  9
#define TFT_CS  10
#define SD_CS   4
#define trigPin 22
#define echoPin 23

// Calibration values
int echoTime       = 23529;
double rangerCalib = 0.76755776;
int rangerOffset   = 0;
// TODO: Allow for SLP entry
float qnh    = 1019.2;

// Hardware declarations
Adafruit_10DOF dof                  = Adafruit_10DOF();
Adafruit_BMP085_Unified bmp         = Adafruit_BMP085_Unified(18001);
Adafruit_ILI9341 tft                = Adafruit_ILI9341(TFT_CS, TFT_DC);
Adafruit_L3GD20_Unified gyro        = Adafruit_L3GD20_Unified(20);
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(30301);
Adafruit_LSM303_Mag_Unified mag     = Adafruit_LSM303_Mag_Unified(30302);
Adafruit_STMPE610 ts                = Adafruit_STMPE610(STMPE_CS);

Ultrasonic ultrasonic(trigPin, echoPin, echoTime);

// Theming
uint16_t darkblue = 0x0126;
uint16_t blue     = 0x01A8;
uint16_t red      = 0xA8C4;
uint16_t orange   = 0xDBC4;
uint16_t cream    = 0xEF37;

bool viewReset = true;

// Previous sensor values
double lastPressure;
double lastTemp;
double lastGyroX;
double lastGyroY;
double lastGyroZ;
double lastAccelX;
double lastAccelY;
double lastAccelZ;
int lastAltitude;
int lastHeading;
int lastPitch = 999;
int lastRange = 999;
int lastRoll = 999;
long totalTime;

char lastTempStr[6];
char lastPressureStr[7];
char lastGyroStr[20];
char lastAccelStr[23];

File logFile;
char logName[11];

// Test the clock by setting the system time to RTC time
void initClock() {
  setSyncProvider(RTC.get);
  if(timeStatus() != timeSet) {
    errorMsg("Error syncing time");
  }
}

void createLogName(int fileCount) {
  sprintf(logName, "log%d.csv", fileCount);
  Serial.println(logName);
}

void initSD() {
  pinMode(TFT_CS, OUTPUT);
  if (!SD.begin(SD_CS)) {
    errorMsg("SD initialisation failed");
  }

  // Create initial file name
  int fileCount = 0;
  createLogName(fileCount);

  bool fileExists = true;

  // Increase file name index until new file is found
  while(fileExists) {
    if (!SD.exists(logName)) {
      logFile = SD.open(logName, FILE_WRITE);
      logFile.println("Time,Pitch,Roll,Heading,Acceleration X (m/s^2),Acceleration Y (m/s^2),Acceleration Z (m/s^2),Gyro X (rad/s),Gyro Y (rad/s),Gyro Z (rad/s),Altitude (ft),Range (cm),Pressure (hPa),Temperature (C)");
      logFile.close();

      fileExists = false;
    } else {
      fileCount++;
      createLogName(fileCount);
    }
  }

  if (!SD.exists(logName)) {
    errorMsg("Log file not created");
  }
}

// Initialise 10DOF Sensors
void init10DOF() {
  if(!accel.begin()) {
    errorMsg("No LSM303 detected");
  }
  if(!mag.begin()) {
    errorMsg("No LSM303 detected");
  }
  if(!bmp.begin()) {
    errorMsg("No BMP180 detected");
  }
  gyro.enableAutoRange(true);
  if(!gyro.begin()) {
    errorMsg("No L3GD20 detected");
  }
}

// Initialise Ranger
void initRanger() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

// Initialise screen
void initScreen() {
  tft.begin();
  tft.setRotation(1);
  clearDisplay();
}

void setup() {
  initScreen();
  initClock();
  init10DOF();
  initRanger();
  initSD();
  navbar();
}


void loop(void) {

  // Get sensor readings
  double rawTemperature = getTemperature();
  double rawPressure    = getPressure();
  double rawAccelX      = getAccel('x');
  double rawAccelY      = getAccel('y');
  double rawAccelZ      = getAccel('z');
  double rawGyroX       = getGyro('x');
  double rawGyroY       = getGyro('y');
  double rawGyroZ       = getGyro('z');
  int altitude          = getAltitude();
  int heading           = getHeading();
  int pitch             = getPitch();
  int range             = getRange();
  int roll              = getRoll();
  time_t timeNow        = RTC.get();

  // Format pressure to string
  char strPressure[7];
  dtostrf(rawPressure, 6, 1, strPressure);
  char *pressure = deblank(strPressure);

  // Format temperature to string
  char strTemperature[6];
  dtostrf(rawTemperature, 5, 1, strTemperature);
  char *temperature = deblank(strTemperature);

  // Format Gyro X to string
  char strGyroX[6];
  dtostrf(rawGyroX, 5, 2, strGyroX);
  char *gyroX = deblank(strGyroX);

  // Format Gyro Y to string
  char strGyroY[6];
  dtostrf(rawGyroY, 5, 2, strGyroY);
  char *gyroY = deblank(strGyroY);

  // Format Gyro Z to string
  char strGyroZ[6];
  dtostrf(rawGyroZ, 5, 2, strGyroZ);
  char *gyroZ = deblank(strGyroZ);

  // Format Acceleration X to string
  char strAccelX[7];
  dtostrf(rawAccelX, 6, 2, strAccelX);
  char *accelX = deblank(strAccelX);

  // Format Acceleration X to string
  char strAccelY[7];
  dtostrf(rawAccelY, 6, 2, strAccelY);
  char *accelY = deblank(strAccelY);

  // Format Acceleration X to string
  char strAccelZ[7];
  dtostrf(rawAccelZ, 6, 2, strAccelZ);
  char *accelZ = deblank(strAccelZ);


  // Log sensor data to SD card
  char logMsg[120];
  sprintf(logMsg, "%02d/%02d/%d %02d:%02d:%02d,%d,%d,%d,%s,%s,%s,%s,%s,%s,%d,%d,%s,%s", day(timeNow), month(timeNow), year(timeNow), hour(timeNow), minute(timeNow), second(timeNow), pitch, roll, heading, accelX, accelY, accelZ, gyroX, gyroY, gyroZ, altitude, range, pressure, temperature);
  logToCard(logMsg);

  // atmosphere(pressure, temperature, rawPressure, rawTemperature);
  ranger(range);

  // // Print Pitch
  // printInt(pitch, &lastPitch, 0, 0, ILI9341_RED, 2);

  // // Print Roll
  // printInt(roll, &lastRoll, 0, 20, ILI9341_RED, 2);

  // // Print Heading
  // printInt(heading, &lastHeading, 0, 40, ILI9341_RED, 2);

  // // Print Altitude
  // printInt(altitude, &lastAltitude, 0, 60, ILI9341_RED, 2);
  
  // // Print Range
  // printInt(range, &lastRange, 0, 80, ILI9341_RED, 2);

  // // Print Accel
  // if (lastAccelX != rawAccelX || lastAccelY != rawAccelY || lastAccelZ != rawAccelZ) {
  //   char accelStr[19];
  //   sprintf(accelStr, "%s, %s, %s", accelX, accelY, accelZ);
  //   printData(accelStr, lastAccelStr, 0, 100, ILI9341_RED, 2);
  //   strcpy(lastAccelStr, accelStr);
  // }

  // // Print Pressure
  // if (lastPressure != rawPressure) {
  //   printData(pressure, lastPressureStr, 0, 140, ILI9341_RED, 2);
  //   strcpy(lastPressureStr, pressure);
  // }

  // // Print Temperature
  // if (lastTemp != rawTemperature) {
  //   printData(temperature, lastTempStr, 0, 160, ILI9341_RED, 2);
  //   strcpy(lastTempStr, temperature);
  // }

  // Update last values
  double lastTemp     = rawTemperature;
  double lastPressure = rawPressure;
  double lastAccelX   = rawAccelX;
  double lastAccelY   = rawAccelY;
  double lastAccelZ   = rawAccelZ;
  double lastGyroX    = rawGyroX;
  double lastGyroY    = rawGyroY;
  double lastGyroZ    = rawGyroZ;
  int lastAltitude    = altitude;
  int lastHeading     = heading;
  int lastPitch       = pitch;
  int lastRange       = range;
  int lastRoll        = roll;

  delay(100);
}


/*
 * Getters
 */

// Get altitude in feet
int getAltitude() {
  sensors_event_t bmp_event;
  bmp.getEvent(&bmp_event);
  if (bmp_event.pressure) {
    // Convert atmospheric pressure, QNH and temp to altitude, then convert to feet
    int altitude = round(
      bmp.pressureToAltitude(
        qnh,
        bmp_event.pressure,
        getTemperature()) * 3.2808399);

    return altitude;
  } else {
    return lastAltitude;
  }
}

// Get heading
int getHeading() {
  sensors_event_t mag_event;
  sensors_vec_t orientation;
  mag.getEvent(&mag_event);
  if (dof.magGetOrientation(SENSOR_AXIS_Z, &mag_event, &orientation)) {
    return (int) orientation.heading;
  } else {
    return lastHeading;
  }
}

// Get ranger distance
int getRange() {
  return (int) ultrasonic.Ranging(CM) * rangerCalib + rangerOffset;
}

// Get roll
int getRoll() {
  sensors_event_t accel_event;
  sensors_event_t mag_event;
  sensors_vec_t orientation;

  accel.getEvent(&accel_event);
  mag.getEvent(&mag_event);
  if (dof.fusionGetOrientation(&accel_event, &mag_event, &orientation)) {
    return (int) orientation.roll;
  } else {
    return lastRoll;
  }
}

// Get pitch
int getPitch() {
  sensors_event_t accel_event;
  sensors_event_t mag_event;
  sensors_vec_t orientation;

  accel.getEvent(&accel_event);
  mag.getEvent(&mag_event);
  if (dof.fusionGetOrientation(&accel_event, &mag_event, &orientation)) {
    return (int) orientation.pitch;
  } else {
    return lastPitch;
  }
}

// Get acceleration
double getAccel(char axis) {
  sensors_event_t event; 
  accel.getEvent(&event);

  switch (axis) {
    case 'x':
      return (double) event.acceleration.x;
      break;
    case 'y':
      return (double) event.acceleration.y;
      break;
    default:
      return (double) event.acceleration.z;
  }
}

// Get rotational velocity
double getGyro(char axis) {
  sensors_event_t event; 
  gyro.getEvent(&event);

  switch (axis) {
    case 'x':
      return (double) event.gyro.x;
      break;
    case 'y':
      return (double) event.gyro.y;
      break;
    default:
      return (double) event.gyro.z;
  }
}

// Get pressure
double getPressure() {
  sensors_event_t event;
  bmp.getEvent(&event);
  if (event.pressure) {
    return (double) event.pressure;
  } else {
    return lastPressure;
  }
}

// Get temperature
double getTemperature() {
  float temperature;
  bmp.getTemperature(&temperature);

  return (double) temperature;
}

/*
 * Views
 */

// Ultrasonic Range
void ranger(int range) {
  // Setup
  if (viewReset) {
    newView(0);
    printData("cm", "", 280, 170, ILI9341_RED, 3);
    incrementBtn(80, 160);
    decrementBtn(20, 160);
    printInt(range, 0, 10, 30, ILI9341_RED, 15);
    viewReset = false;
  }

  // Loop
  if (lastRange != range) {
    printInt(range, &lastRange, 10, 30, ILI9341_RED, 15);
  }
}

// Heading, Pitch, Roll & Altitude
void attitude(int heading, char qnhStr[], int altitude, int pitch, int roll) {
  // Setup
  if (viewReset) {
    newView(1);
    decrementBtn(10, 68);
    decrementBtn(10, 98);
    decrementBtn(10, 128);
    incrementBtn(150, 68);
    incrementBtn(150, 98);
    incrementBtn(150, 128);
    printData("Pitch", "", 40, 100, ILI9341_RED, 3);
    printData("Roll", "", 40, 130, ILI9341_RED, 3);
    viewReset = false;
  }

  // Loop

  printInt(heading, &lastHeading, 125, 10, ILI9341_RED, 4);

  printData(qnhStr, "", 40, 70, ILI9341_RED, 3);
  printInt(altitude, &lastAltitude, 190, 70, ILI9341_RED, 3);
  printInt(pitch, &lastPitch, 190, 100, ILI9341_RED, 3);
  printInt(roll, &lastRoll, 190, 130, ILI9341_RED, 3);
}

// Pressure and Temp
void atmosphere(char pressure[], char temperature[], double pressureVal, double temperatureVal) {
  if (viewReset) {
    newView(2);
    printData("Pressure", "", 10, 10, ILI9341_RED, 3);
    printData("Temperature", "", 10, 110, ILI9341_RED, 3);
    printData(pressure, "", 10, 40, ILI9341_RED, 4);
    printData(temperature, "", 10, 140, ILI9341_RED, 4);
    viewReset = false;
  }

  if ((int) round(lastPressure * 10) != (int) round(pressureVal * 10)) {
    printData(pressure, lastPressureStr, 10, 40, ILI9341_RED, 4);
    strcpy(lastPressureStr, pressure);
    lastPressure = pressureVal;
  }
  
  if ((int) round(lastTemp * 10) != (int) round(temperatureVal * 10)) {
    printData(temperature, lastTempStr, 10, 140, ILI9341_RED, 4);
    strcpy(lastTempStr, temperature);
    lastTemp = temperatureVal;
  }

}

/*
 * Outputs
 */

 // Fill the screen black
void clearDisplay() {
  tft.fillScreen(ILI9341_BLACK);
}

// Fill navbar
void navbar() {
  tft.fillRect(0, 198, 320, 42, darkblue);
}

// Blank display for new view
void newView(int menu) {
  tft.fillRect(0, 198, 320, 42, darkblue);
  tft.fillRect(menu * 64, 198, 64, 42, orange);
  viewReset = false;
}

// Print integer
void printInt(int msg, int *lastMsg, int x, int y, uint16_t color, int textSize) {
  if (*lastMsg != msg) {
    char lastMsgStr[6];
    sprintf(lastMsgStr, "%d", *lastMsg);
    char msgStr[6];
    sprintf(msgStr, "%d", msg);
    printData(msgStr, lastMsgStr, x, y, ILI9341_RED, textSize);
    *lastMsg = msg;
  }
}

// Print Data
void printData(char msg[], char lastMsg[], int x, int y,  uint16_t color, int textSize) {
  tft.setCursor(x, y);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(textSize);
  tft.println(lastMsg);
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.setTextSize(textSize);
  tft.println(msg);
}

// Draw decrement button
void decrementBtn(int x, int y) {
  tft.fillRect(x, y, 25, 25, red);
  tft.fillRect(x + 2, y + 12, 21, 2, ILI9341_WHITE);
}

// Draw increment button
void incrementBtn(int x, int y) {
  tft.fillRect(x, y, 25, 25, red);
  tft.fillRect(x + 12, y + 2, 2, 21, ILI9341_WHITE);
  tft.fillRect(x + 2, y + 12, 21, 2, ILI9341_WHITE);
}

// Report error message
void errorMsg(char msg[]) {
  clearDisplay();
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_RED);                  
  tft.setTextSize(2);
  tft.println(msg);

  while(1);
}

// Log to SD
void logToCard(char msg[]) {
  if (millis() >= totalTime + 1000) {
    logFile = SD.open(logName, FILE_WRITE);

    if (logFile) {
      logFile.println(msg);
      logFile.close();
    }
    totalTime = millis();
  }
}

// Remove spaces from string
char *deblank(char *str) {
  char *out = str, *put = str;

  for(; *str != '\0'; ++str) {
    if(*str != ' ')
      *put++ = *str;
  }
  *put = '\0';

  return out;
}