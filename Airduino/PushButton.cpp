/*
 * PushButton.cpp
 * Simple pushbutton object
 * RSA Final Project
 */

#include "Arduino.h"
#include "PushButton.h"

PushButton::PushButton(byte buttonPin) {
  pinMode(buttonPin, INPUT_PULLUP);
  _buttonPin = buttonPin;
  _lastPressedMs = 0;
}

bool PushButton::isPressed() {
  // Have a delay of buttonDelayMs.
  // In other words, if called within buttonDelayMs of previous call,
  // return false.
  if (millis() - _lastPressedMs > buttonDelayMs) {
    if (digitalRead(_buttonPin) == LOW) {
      _lastPressedMs = millis();
      return true;
    }
  }
  return false;
}
