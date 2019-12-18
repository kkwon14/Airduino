/*
 * PushButton.h
 * Simple pushbutton object
 * RSA Final Project
 */
#pragma once

#include "Arduino.h"

static const uint8_t buttonDelayMs = 250;

class PushButton {
  public:
    PushButton(byte buttonPin);
    bool isPressed();
  private:
    byte _buttonPin;
    unsigned long _lastPressedMs;
};
