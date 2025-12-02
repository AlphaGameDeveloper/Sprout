// Copyright (c) 2025 Damien Boisvert (AlphaGameDeveloper)
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <Arduino.h>

// Use the board-defined LED pin when available; fall back to GPIO2 which is
// the on-board LED on many ESP32 development boards.
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// Blink interval in milliseconds
static const unsigned long BLINK_MS = 1000UL;

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
    digitalWrite(LED_BUILTIN, HIGH);
    delay(BLINK_MS);
    digitalWrite(LED_BUILTIN, LOW);
    delay(BLINK_MS);
}