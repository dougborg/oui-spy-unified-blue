#pragma once

#include <cstdint>

namespace hal::neopixel_logic {

struct BreathingState {
    float brightness;
    bool increasing;
};

struct FlashFrame {
    bool active;
    int frameIndex;
    uint8_t value;
};

BreathingState nextBreathing(BreathingState current);
FlashFrame computeFlashFrame(unsigned long elapsedMs, uint8_t brightValue, uint8_t dimValue);

} // namespace hal::neopixel_logic
