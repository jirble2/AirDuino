#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "SPI.h"
#include <Adafruit_10DOF.h>
#include <Adafruit_BMP085_U.h>
#include <Adafruit_L3GD20_U.h>
#include <Adafruit_LSM303_U.h>
#include <Adafruit_Sensor.h>
#include <math.h>
#include <SD.h>
#include <Wire.h>

// Pin declarations
#define TFT_DC  9
#define TFT_CS  10
#define SD_CS   4
#define trigPin 30
#define echoPin 32

// Hardware declarations
Adafruit_10DOF dof                  = Adafruit_10DOF();
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(30301);
Adafruit_LSM303_Mag_Unified mag     = Adafruit_LSM303_Mag_Unified(30302);
Adafruit_BMP085_Unified bmp         = Adafruit_BMP085_Unified(18001);
Adafruit_ILI9341 tft                = Adafruit_ILI9341(TFT_CS, TFT_DC);

// TODO: Allow for SLP entry
float seaLevelPressure = 1028.4;

// Previous sensor values
double lastTemp  = 0;
int lastAltitude = 0;
int lastHeading  = 0;
int lastRange    = 0;
long totalTime   = 0;

char lastTempStr[6];

File logFile;
char logName[11];

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
      logFile.println("Temperature (C),Altitude (ft),Heading,Range (cm)");
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

void initSensors() {
  if(!accel.begin()) {
    /* There was a problem detecting the LSM303 ... check your connections */
    Serial.println(F("Ooops, no LSM303 detected ... Check your wiring!"));
    while(1);
  }
  if(!mag.begin()) {
    /* There was a problem detecting the LSM303 ... check your connections */
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while(1);
  }
  if(!bmp.begin()) {
    /* There was a problem detecting the BMP180 ... check your connections */
    Serial.println("Ooops, no BMP180 detected ... Check your wiring!");
    while(1);
  }
}

void setup() {
  Serial.begin(9600);
  tft.begin();
  tft.setRotation(1);
  initSD();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  initSensors();
  clearDisplay();
}


void loop(void) {
  sensors_event_t accel_event;
  char stringTemperature[6];
  double temperature = getTemperature();
  int altitude       = getAltitude();
  int heading        = getHeading();
  int range          = getRange();

  dtostrf(temperature, 5, 1, stringTemperature);
  char * cleanTemp = deblank(stringTemperature);

  char logMsg[20];

  sprintf(logMsg, "%s,%d,%d,%d", stringTemperature, altitude, heading, range);
  logToCard(logMsg);

  // Print Range
  if (lastRange != range) {
    char lastRangeStr[4];
    sprintf(lastRangeStr, "%d", lastRange);
    char rangeStr[4];
    sprintf(rangeStr, "%d", range);
    printData(rangeStr, lastRangeStr, 0, 0, ILI9341_RED, 6);
    lastRange = range;
  }

  // Print Altitude
  if (lastAltitude != altitude) {
    char lastAltitudeStr[6];
    sprintf(lastAltitudeStr, "%d", lastAltitude);
    char altitudeStr[6];
    sprintf(altitudeStr, "%d", altitude);
    printData(altitudeStr, lastAltitudeStr, 0, 60, ILI9341_RED, 6);
    lastAltitude = altitude;
  }

  // Print Heading
  if (lastHeading != heading) {
    char lastHeadingStr[4];
    sprintf(lastHeadingStr, "%d", lastHeading);
    char headingStr[4];
    sprintf(headingStr, "%d", heading);
    printData(headingStr, lastHeadingStr, 0, 120, ILI9341_RED, 6);
    lastHeading = heading;
  }

  // Print Temperature
  if (lastTemp != temperature) {
    printData(cleanTemp, lastTempStr, 0, 180, ILI9341_RED, 6);
    strcpy(lastTempStr, cleanTemp);
    lastTemp = temperature;
  }

  delay(250);
}


/*
 * Inputs
 */

// Get altitude in feet
int getAltitude() {
  sensors_event_t bmp_event;
  bmp.getEvent(&bmp_event);
  if (bmp_event.pressure) {
    // Convert atmospheric pressure, SLP and temp to altitude, then convert to feet
    int altitude = round(
      bmp.pressureToAltitude(
        seaLevelPressure,
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
  long duration;
  long distance;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration/2) / 29.1;
  if (distance >= 200 || distance <= 0){
    return (int) -1;
  }
  else {
    return (int) distance;
  }
}

// Get temperature
double getTemperature() {
  float temperature;
  bmp.getTemperature(&temperature);

  return (double) temperature;
}


/*
 * Outputs
 */

// Fill the screen black
void clearDisplay() {
  tft.fillScreen(ILI9341_BLACK);
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
char * deblank(char *str) {
  char *out = str, *put = str;

  for(; *str != '\0'; ++str) {
    if(*str != ' ')
      *put++ = *str;
  }
  *put = '\0';

  return out;
}