#pragma once

#include "board.h"
#include <Arduino.h>

// ============================================================================
// LED Control — adapts to board-specific logic
// ============================================================================

namespace hal {

#if HAS_LED

inline void ledInit() {
    pinMode(LED_PIN, OUTPUT);
#if LED_ACTIVE_LOW
    digitalWrite(LED_PIN, HIGH); // OFF
#else
    digitalWrite(LED_PIN, LOW); // OFF
#endif
}

inline void ledOn() {
#if LED_ACTIVE_LOW
    digitalWrite(LED_PIN, LOW);
#else
    digitalWrite(LED_PIN, HIGH);
#endif
}

inline void ledOff() {
#if LED_ACTIVE_LOW
    digitalWrite(LED_PIN, HIGH);
#else
    digitalWrite(LED_PIN, LOW);
#endif
}

#else // !HAS_LED

inline void ledInit() {}
inline void ledOn() {}
inline void ledOff() {}

#endif

} // namespace hal
