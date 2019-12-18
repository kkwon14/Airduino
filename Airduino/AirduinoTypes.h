/*
 * AirduinoTypes.h
 * Some structs/enums and functions for those types used for the Airduino
 * RSA Final Project
 */

#pragma once

#include <EEPROM.h>
#include <EEWrap.h>

enum basicModes {
  basicInitialize,
  autoMode,
  onMode,
  offMode
};

enum autoModes {
  autoInitialize,
  autoOn,
  autoOff,
  editHour,
  editMinute,
  editType,
  editValue
};

enum autoTypes {
  temperature,
  humidity,
  apparentTemperature
};

struct autoModeParameters {
  uint8_e hour;
  uint8_e minute;
  // A hack to get type to be stored in EEPROM correctly. Should really be of type autoTypes.
  uint8_e type;
  int16_e value;
};

// Increments or decrements an integer circularly.
// param n The integer being incremented/decremented
// param minN The minimum value of n
// param maxN The maximum value of n
// param isIncrement True if incrementing, false if decrementing
// returns the new value of the integer.
static uint8_t incrementDecrementCircular(uint8_t n, uint8_t minN, uint8_t maxN, bool isIncrement) {
  if (isIncrement) {
    if (n == maxN) { // circle back around
      return minN;
    } else {
      return (n + 1);
    }
  } else { // decrement
    if (n == minN) { // circle back around
      return maxN;
    } else {
      return (n - 1);
    }
  }
}

static uint8_t incrementHour(uint8_t currHour) {
  return incrementDecrementCircular(currHour, 0, 23, true);
}

static uint8_t decrementHour(uint8_t currHour) {
  return incrementDecrementCircular(currHour, 0, 23, false);
}

static uint8_t incrementMinute(uint8_t currMinute) {
  return incrementDecrementCircular(currMinute, 0, 59, true);
}

static uint8_t decrementMinute(uint8_t currMinute) {
  return incrementDecrementCircular(currMinute, 0, 59, false);
}

static uint8_t incrementType(uint8_t type) {
  return incrementDecrementCircular(type, 0, 2, true);
}

static uint8_t decrementType(uint8_t type) {
  return incrementDecrementCircular(type, 0, 2, false);
}

// Converts the autoTypes type to its corresponding String.
static String autoTypesToString(uint8_t type) {
  switch ((autoTypes)type) {
    case temperature:
      return "Temperature";
    case humidity:
      return "Humidity";
    case apparentTemperature:
      return "Apparent Temperature";
    default: // needs to be reset
      return "Invalid - Please press reset.";
  }
}

// Because of the hack of keeping type as a uint8_t, converts to enum
static autoTypes typeToEnum(uint8_t type) {
  switch (type) {
    case 0:
      return temperature;
    case 1:
      return humidity;
    case 2:
      return apparentTemperature;
    default: // needs to be reset - just return something
      return temperature;
  }
}

// returns 1 if time1 > time2, 0 if =, -1 if time1 < time2
static int8_t compareTime(uint8_t time1Hour, uint8_t time1Minute, uint8_t time2Hour, uint8_t time2Minute) {
  if (time1Hour > time2Hour) {
    // Hours more significant than minute, can ignore minutes
    return 1;
  } else if (time1Hour < time2Hour) {
    return -1;
  } else { // equal hour
    if (time1Minute > time2Minute) {
      return 1;
    } else if (time1Minute < time2Minute) {
      return -1;
    } else { // Hour and minute are equal
      return 0;
    }
  }
}
