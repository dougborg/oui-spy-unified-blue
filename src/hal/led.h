#pragma once

#include "pins.h"
#include <Arduino.h>

// ============================================================================
// LED Control (XIAO ESP32-S3 inverted logic: LOW=ON, HIGH=OFF)
// ============================================================================

namespace hal {

inline void ledInit() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // OFF
}

inline void ledOn() {
    digitalWrite(LED_PIN, LOW);
}

inline void ledOff() {
    digitalWrite(LED_PIN, HIGH);
}

} // namespace hal
