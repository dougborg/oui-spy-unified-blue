#include "neopixel_logic.h"

namespace hal::neopixel_logic {

BreathingState nextBreathing(BreathingState current) {
    if (current.increasing) {
        current.brightness += 0.02f;
        if (current.brightness >= 1.0f) {
            current.brightness = 1.0f;
            current.increasing = false;
        }
    } else {
        current.brightness -= 0.02f;
        if (current.brightness <= 0.1f) {
            current.brightness = 0.1f;
            current.increasing = true;
        }
    }
    return current;
}

FlashFrame computeFlashFrame(unsigned long elapsedMs, uint8_t brightValue, uint8_t dimValue) {
    int frame = (int)(elapsedMs / 250UL);
    if (frame >= 3)
        return {false, frame, 0};

    bool bright = (elapsedMs % 250UL) < 150UL;
    return {true, frame, bright ? brightValue : dimValue};
}

} // namespace hal::neopixel_logic
