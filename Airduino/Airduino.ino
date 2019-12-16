/*
 * Turns an aircon on/off based on several factors.
 * RSA Final Project
 */

#include <EEPROM.h>
#include <EEWrap.h>
#include <DHT.h>
#include <RTClib.h>
#include <Servo.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "PushButton.h"
#include "AirduinoTypes.h"

#include "PushButton.h"
#include "AirduinoTypes.h"

#define DHTTYPE DHT11 // DHT 11 temp/hum sensor
#define OLED_RESET     -1
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

const byte dhtPin = 2; // dht temp/humidity pin
const byte toggleButtonPin = 3;
const byte upButtonPin = 4;
const byte downButtonPin = 5;
const byte editButtonPin = 6;
const byte resetButtonPin = 7;
const byte motorPin = 9;
int pos = 0;

// Initialize pushbuttons.
PushButton toggleButton(toggleButtonPin);
PushButton upButton(upButtonPin);
PushButton downButton(downButtonPin);
PushButton editButton(editButtonPin);

// Initialize DHT sensor.
DHT dht(dhtPin, DHTTYPE);

//Initialize RTC.
RTC_PCF8523 rtc;

// Initialize Display.
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Initialize Servomotor.
Servo myServo;

// Airduino mode (auto, on, off)
basicModes currBasicMode;
autoModes currAutoMode = autoInitialize;
autoModeParameters autoParams EEMEM;

// Current measurements
float currTemperature;
float currHumidity;
float currApparentTemperature;
int currHour;
int currMinute;

// Current aircon state
bool isAirconOn = false;

void setup() {
  Serial.begin(57600);
  // Set RTC
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (! rtc.initialized()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  dht.begin();
  myServo.attach(motorPin); //PWM

  pinMode(resetButtonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(resetButtonPin),
                  reset,
                  FALLING);

  pinMode(motorPin, OUTPUT);
  digitalWrite(motorPin, LOW);
  display.display();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();
}

void loop() {
  updateMeasurements();
  doBasicFSM();
}

// Updates currTemperature, currHumidity, currApparentTemperature, currHour, currMinute
static void updateMeasurements() {
  currTemperature = dht.readTemperature(true); // isFahrenheit = true
  currHumidity = dht.readHumidity();
  currApparentTemperature = dht.computeHeatIndex(currTemperature, currHumidity, true); // isFahrenheit = true

  DateTime now = rtc.now();
  currHour = now.hour();
  currMinute = now.minute();
}

// was current measurements
static void displayMeasurements(String mode) {
  display.display();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(mode);
  display.println("     Time: " + convertTime(currHour, currMinute));
  display.println("Temperature  : " + (String)currTemperature);
  display.println("Humidity     : " + (String)currHumidity);
  display.print("Apparent Temp: " + (String)currApparentTemperature + "       ");
  display.display();
}

// Converts time to string format
static String convertTime(int hour, int minute) {
  String timeString = (String)hour + ":";
  if (minute < 10) {
    timeString += "0";
  }
  timeString += (String)minute;
  return timeString;
}

static void doBasicFSM() {
  switch (currBasicMode) {
    case basicInitialize:
      currBasicMode = autoMode;
      break;
      
    case autoMode: // Automatically determine if on/off
      doAutoFSM();
      if (toggleButton.isPressed()) {
        currBasicMode = onMode;
      } else {
        currBasicMode = autoMode;
      }
      
      break;
      
    case onMode: // Always on
      displayMeasurements("On  ");
      turnOn();
      if (toggleButton.isPressed()) {
        currBasicMode = offMode;
      } else {
        currBasicMode = onMode;
      }
      break;
      
    case offMode: // Always off
      displayMeasurements("Off ");
      turnOff();
      if (toggleButton.isPressed()) {
        currBasicMode = autoMode;
      } else {
        currBasicMode = offMode;
      }
      break;
  }
}

static void doAutoFSM() {
  switch (currAutoMode) {
    case autoInitialize:
      checkAutoCondition();
      break;
      
    case autoOn:
      displayMeasurements("AOn ");
      turnOn();
      checkAutoCondition();
      // Check for next condition
      if (editButton.isPressed()) {
        currAutoMode = editHour;
      }
      break;
      
    case autoOff:
      displayMeasurements("AOff");
      turnOff();
      checkAutoCondition();
      // Check for next condition
      if (editButton.isPressed()) {
        currAutoMode = editHour;
      }
      break;
      
    case editHour:
      // Adjust hour
      if (upButton.isPressed()) {
        autoParams.hour = incrementHour(autoParams.hour);
      }
      if (downButton.isPressed()) {
        autoParams.hour = decrementHour(autoParams.hour);
      }
      // Check for next condition
      if (editButton.isPressed()) {
        currAutoMode = editMinute;
      }
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Editing Hour");
      display.println((String)autoParams.hour);
      display.display();
      break;
      
    case editMinute:
      // Adjust minute
      if (upButton.isPressed()) {
        autoParams.minute = incrementMinute(autoParams.minute);
      }
      if (downButton.isPressed()) {
        autoParams.minute = decrementMinute(autoParams.minute);
      }
      // Check for next condition
      if (editButton.isPressed()) {
        currAutoMode = editType;
      }
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Editing Minute");
      display.println((String)autoParams.minute);
      display.display();
      
      break;
      
    case editType:
      // Adjust type
      if (upButton.isPressed()) {
        autoParams.type = incrementType(autoParams.type);
      }
      if (downButton.isPressed()) {
        autoParams.type = decrementType(autoParams.type);
      }
      // Check for next condition
      if (editButton.isPressed()) {
        currAutoMode = editValue;
      }
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Editing Mode");
      display.println(autoTypesToString(autoParams.type));
      display.display();
      
      break;
      
    case editValue:
      // Adjust type
      if (upButton.isPressed()) {
        autoParams.value++;
      }
      if (downButton.isPressed()) {
        autoParams.value--;
      }
      // Check for next condition
      if (editButton.isPressed()) {
        currAutoMode = autoInitialize;
      }
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Editing Value");
      display.println(autoParams.value);
      display.display();
      
      break;
  }
}

// Checks if aircon should be on or off in auto mode and adjusts currAutoMode accordignly
static void checkAutoCondition() {
  // Check time and appropriate condition (temp, hum, ap. temp)

  DateTime now = rtc.now();

  
  // Check time: if current time >= set time
  if (compareTime(currHour, currMinute, autoParams.hour, autoParams.minute) != -1) {
    // Check condition
    switch (typeToEnum(autoParams.type)) {
      case temperature:
        if (currTemperature > autoParams.value) { // turn on
          turnOn();
          currAutoMode = autoOn;
        } else {
          turnOff();
          currAutoMode = autoOff;
        }
        break;
        
      case humidity:
        if (currHumidity > autoParams.value) { // turn on
          turnOn();
          currAutoMode = autoOn;
        } else {
          turnOff();
          currAutoMode = autoOff;
        }
        break;
        
      case apparentTemperature:
        if (currApparentTemperature > autoParams.value) { // turn on
          turnOn();
          currAutoMode = autoOn;
        } else {
          turnOff();
          currAutoMode = autoOff;
        }
        break;
        
      default: // not initialized - need to be reset
        turnOff();
        currAutoMode = autoOff;
        // turn off when nothing specified
    }
  } else { // Not correct time - turn off
    turnOff();
    currAutoMode = autoOff;
  }
}

// A function used for testing prints parameters
static void printParams() {
  Serial.print(autoParams.hour);
  Serial.print(" ");
  Serial.print(autoParams.minute);
  Serial.print(" ");
  Serial.print(autoTypesToString(autoParams.type));
  Serial.print(" ");
  Serial.println(autoParams.value);
}

// Resets to defaults.
// Will need to be run when starting for the first time to initialize the EEPROM values.
void reset() {
  currBasicMode = basicInitialize;
  currAutoMode = autoInitialize;
  autoParams.hour = 0;
  autoParams.minute = 0;
  autoParams.type = temperature;
  autoParams.value = 0;
}

static void turnOn() {
  if (!isAirconOn) {
    toggleMotor();
    isAirconOn = true;
  }
}

static void turnOff() {
  if (isAirconOn) {
    toggleMotor();
    isAirconOn = false;
  }
}

static void toggleMotor() {
    for (int pos = 0; pos <= 40; pos += 1) { // servo pushes AC "ON" button
    // in steps of 1 degree
    myServo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
  delay(150);
  for (int pos = 40; pos <= 0; pos -= 5) { // servo pushes AC "ON" button
    // in steps of 1 degree
    myServo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
  delay(200);
}
