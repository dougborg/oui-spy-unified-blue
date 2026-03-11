#pragma once

#include <cstdint>

namespace hal::neopixel_logic {

struct FlashFrame {
    bool active;
    int frameIndex;
    uint8_t value;
};

// Sine-wave breathing: returns brightness value (15–60) for smooth animation.
// Period is 1500ms. Pass current millis() as nowMs.
uint8_t breathingValue(uint32_t nowMs);

FlashFrame computeFlashFrame(unsigned long elapsedMs, uint8_t brightValue, uint8_t dimValue);

} // namespace hal::neopixel_logic
