/*
 * Turns an aircon on/off based on several factors.
 * RSA Final Project
 */

#include <EEPROM.h>
#include <EEWrap.h>
#include <DHT.h>

#include "PushButton.h"
#include "AirduinoTypes.h"

#define DHTTYPE DHT11 // DHT 11 temp/hum sensor

const byte dhtPin = 2; // dht temp/humidity pin
const byte toggleButtonPin = 3;
const byte upButtonPin = 4;
const byte downButtonPin = 5;
const byte editButtonPin = 6;
const byte resetButtonPin = 7;

// TODO Motor Remove temp LED tester.
const byte tempLEDTesterPin = A3;

// Initialize pushbuttons.
PushButton toggleButton(toggleButtonPin);
PushButton upButton(upButtonPin);
PushButton downButton(downButtonPin);
PushButton editButton(editButtonPin);

// Initialize DHT sensor.
DHT dht(dhtPin, DHTTYPE);

// Airduino mode (auto, on, off)
basicModes currBasicMode;
autoModes currAutoMode = autoInitialize;
autoModeParameters autoParams EEMEM;

// Current measurements
float currTemperature;
float currHumidity;
float currApparentTemperature;

void setup() {
  Serial.begin(9600);

  dht.begin();

  pinMode(resetButtonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(resetButtonPin),
                  reset,
                  FALLING);

  // TODO Motor Remove temp LED tester
  pinMode(tempLEDTesterPin, OUTPUT);
  digitalWrite(tempLEDTesterPin, LOW);
}

void loop() {
  updateMeasurements();
  doBasicFSM();
}

// Updates currTemperature, currHumidity, currApparentTemperature
static void updateMeasurements() {
  currTemperature = dht.readTemperature(true); // isFahrenheit = true
  currHumidity = dht.readHumidity();
  currApparentTemperature = dht.computeHeatIndex(currTemperature, currHumidity, true); // isFahrenheit = true
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
      turnOn();
      if (toggleButton.isPressed()) {
        currBasicMode = offMode;
      } else {
        currBasicMode = onMode;
      }
      break;
      
    case offMode: // Always off
      turnOff();
      if (toggleButton.isPressed()) {
        currBasicMode = autoMode;
      } else {
        currBasicMode = offMode;
      }
      break;
  }
  // TODO Remove test print
  Serial.println(currBasicMode);
}

static void doAutoFSM() {
  switch (currAutoMode) {
    case autoInitialize:
      checkAutoCondition();
      break;
      
    case autoOn:
      turnOn();
      checkAutoCondition();
      // Check for next condition
      if (editButton.isPressed()) {
        currAutoMode = editHour;
      }
      break;
      
    case autoOff:
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
      break;
  }

  // TODO remove test print
  Serial.println(currAutoMode);
  printParams();
}

// Checks if aircon should be on or off in auto mode and adjusts currAutoMode accordignly
static void checkAutoCondition() {
  // Check time and appropriate condition (temp, hum, ap. temp)
  // Check time: if current time >= set time
  if (compareTime(getHour(), getMinute(), autoParams.hour, autoParams.minute) != -1) {
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


// TODO Remove temporary fake tester functions
static int getHour() {
  return 5;
}

static int getMinute() {
  return 10;
}

static void turnOn() {
  digitalWrite(tempLEDTesterPin, HIGH);
}
static void turnOff() {
  digitalWrite(tempLEDTesterPin, LOW);
}
