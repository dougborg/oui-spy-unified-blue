#include "neopixel_logic.h"
#include <cmath>

namespace hal::neopixel_logic {

uint8_t breathingValue(uint32_t nowMs) {
    // Time-based sine wave: 1500ms full cycle (~0.67Hz breathing rate)
    // Output range: 15–60 (avoids going fully dark, stays visible)
    float phase = (nowMs % 1500u) / 1500.0f * 2.0f * 3.14159265f;
    float val = (std::sin(phase) + 1.0f) / 2.0f; // 0.0–1.0
    return static_cast<uint8_t>(15 + val * 45);  // 15–60
}

FlashFrame computeFlashFrame(unsigned long elapsedMs, uint8_t brightValue, uint8_t dimValue) {
    int frame = (int)(elapsedMs / 250UL);
    if (frame >= 3)
        return {false, frame, 0};

    bool bright = (elapsedMs % 250UL) < 150UL;
    return {true, frame, bright ? brightValue : dimValue};
}

} // namespace hal::neopixel_logic
