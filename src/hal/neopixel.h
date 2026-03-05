#pragma once

#include "board.h"
#include <Arduino.h>

#if HAS_NEOPIXEL
#include <Adafruit_NeoPixel.h>
#elif HAS_APA102
#include <Adafruit_DotStar.h>
#endif

// ============================================================================
// Shared NeoPixel HAL with priority-based animation states
// Backends: WS2812 (NeoPixel), APA102 (DotStar), plain LED, or no-op
// ============================================================================

namespace hal {

// Animation states (ordered by priority, highest wins)
enum NeoPixelState : uint8_t {
    NEO_IDLE_BREATHING = 0,
    NEO_HEARTBEAT_GLOW = 1,
    NEO_DETECTION_FLASH = 2,
    NEO_SCAN_MODE_CYCLE = 3,
};

void neopixelInit();
void neopixelUpdate();

void neopixelSetState(NeoPixelState state, uint16_t hue = 300);
void neopixelFlash(uint16_t hue1 = 240, uint16_t hue2 = 300, uint16_t hue3 = 270);
void neopixelSetIdleHue(uint16_t hue);
void neopixelStartScanAnimation();
void neopixelSetColor(uint8_t r, uint8_t g, uint8_t b);
void neopixelOff();

uint32_t hsvToRgb(uint16_t h, uint8_t s, uint8_t v);

#if HAS_NEOPIXEL
Adafruit_NeoPixel& neopixelStrip();
#elif HAS_APA102
Adafruit_DotStar& neopixelStrip();
#endif

} // namespace hal
