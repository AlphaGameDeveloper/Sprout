// Copyright (c) 2025 Damien Boisvert (AlphaGameDeveloper)
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <Arduino.h>
#include "Logger.h"
// Use the board-defined LED pin when available; fall back to GPIO2 which is
// the on-board LED on many ESP32 development boards.
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// Blink interval in milliseconds
static const unsigned long BLINK_MS         = 1000UL;
static const unsigned long SERIAL_BAUD_RATE = 115200;

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
    L_INFO("Blinking the LED");
    digitalWrite(LED_BUILTIN, HIGH);
    delay(BLINK_MS);
    digitalWrite(LED_BUILTIN, LOW);
    delay(BLINK_MS);
}