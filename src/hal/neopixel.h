#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "pins.h"

// ============================================================================
// Shared NeoPixel HAL with priority-based animation states
// ============================================================================

namespace hal {

// Animation states (ordered by priority, highest wins)
enum NeoPixelState : uint8_t {
    NEO_IDLE_BREATHING = 0,  // Default: slow breathing
    NEO_HEARTBEAT_GLOW = 1,  // Device in range: dim steady glow
    NEO_DETECTION_FLASH = 2, // Detection alert: rapid color flashes
};

void neopixelInit();
void neopixelUpdate(); // Call every loop iteration

// Set the animation state; higher priority overrides lower
void neopixelSetState(NeoPixelState state, uint16_t hue = 300);

// Trigger a detection flash (auto-reverts to previous state after ~750ms)
void neopixelFlash(uint16_t hue1 = 240, uint16_t hue2 = 300, uint16_t hue3 = 270);

// Set idle breathing hue (per-module customization)
void neopixelSetIdleHue(uint16_t hue);

// Direct color control
void neopixelSetColor(uint8_t r, uint8_t g, uint8_t b);
void neopixelOff();

// HSV to RGB conversion (shared utility)
uint32_t hsvToRgb(uint16_t h, uint8_t s, uint8_t v);

// Access the strip object for advanced use
Adafruit_NeoPixel& neopixelStrip();

} // namespace hal
