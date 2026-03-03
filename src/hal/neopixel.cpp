#include "neopixel.h"
#include "neopixel_logic.h"

namespace hal {

#define NEOPIXEL_BRIGHTNESS 50
#define NEOPIXEL_DETECTION_BRIGHTNESS 200

static Adafruit_NeoPixel _strip(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

static NeoPixelState _state = NEO_IDLE_BREATHING;
static uint16_t _idleHue = 300; // Default: pink
static uint16_t _heartbeatHue = 300;

// Flash state
static bool _flashing = false;
static unsigned long _flashStart = 0;
static uint16_t _flashHues[3] = {240, 300, 270};

// Breathing state
static unsigned long _breatheLastUpdate = 0;
static float _breatheBrightness = 0.0;
static bool _breatheIncreasing = true;

Adafruit_NeoPixel& neopixelStrip() {
    return _strip;
}

uint32_t hsvToRgb(uint16_t h, uint8_t s, uint8_t v) {
    uint8_t r, g, b;
    if (s == 0) {
        r = g = b = v;
    } else {
        // Normalize h from 0-360 to 0-255
        uint8_t h8 = (uint8_t)((uint32_t)h * 255 / 360);
        uint8_t region = h8 / 43;
        uint8_t remainder = (h8 - (region * 43)) * 6;
        uint8_t p = (v * (255 - s)) >> 8;
        uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
        uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
        switch (region) {
        case 0:
            r = v;
            g = t;
            b = p;
            break;
        case 1:
            r = q;
            g = v;
            b = p;
            break;
        case 2:
            r = p;
            g = v;
            b = t;
            break;
        case 3:
            r = p;
            g = q;
            b = v;
            break;
        case 4:
            r = t;
            g = p;
            b = v;
            break;
        default:
            r = v;
            g = p;
            b = q;
            break;
        }
    }
    return _strip.Color(r, g, b);
}

void neopixelInit() {
    _strip.begin();
    _strip.setBrightness(NEOPIXEL_BRIGHTNESS);
    _strip.clear();
    _strip.show();
    Serial.println("[HAL] NeoPixel initialized");
}

void neopixelSetIdleHue(uint16_t hue) {
    _idleHue = hue;
}

void neopixelSetState(NeoPixelState state, uint16_t hue) {
    _state = state;
    if (state == NEO_HEARTBEAT_GLOW) {
        _heartbeatHue = hue;
    }
}

void neopixelFlash(uint16_t hue1, uint16_t hue2, uint16_t hue3) {
    _flashing = true;
    _flashStart = millis();
    _flashHues[0] = hue1;
    _flashHues[1] = hue2;
    _flashHues[2] = hue3;
}

void neopixelSetColor(uint8_t r, uint8_t g, uint8_t b) {
    _strip.setPixelColor(0, _strip.Color(r, g, b));
    _strip.show();
}

void neopixelOff() {
    _strip.clear();
    _strip.show();
}

// Breathing animation
static void updateBreathing() {
    if (millis() - _breatheLastUpdate < 20)
        return;
    _breatheLastUpdate = millis();

    neopixel_logic::BreathingState state{_breatheBrightness, _breatheIncreasing};
    state = neopixel_logic::nextBreathing(state);
    _breatheBrightness = state.brightness;
    _breatheIncreasing = state.increasing;

    uint32_t color = hsvToRgb(_idleHue, 255, (uint8_t)(NEOPIXEL_BRIGHTNESS * _breatheBrightness));
    _strip.setPixelColor(0, color);
    _strip.show();
}

// Detection flash animation
static void updateFlash() {
    unsigned long elapsed = millis() - _flashStart;
    auto frame = neopixel_logic::computeFlashFrame(elapsed, NEOPIXEL_DETECTION_BRIGHTNESS,
                                                   (NEOPIXEL_BRIGHTNESS / 4));
    if (!frame.active) {
        _flashing = false;
        return;
    }
    _strip.setPixelColor(0, hsvToRgb(_flashHues[frame.frameIndex], 255, frame.value));
    _strip.show();
}

// Heartbeat glow
static void updateHeartbeat() {
    _strip.setPixelColor(0, hsvToRgb(_heartbeatHue, 255, 30));
    _strip.show();
}

void neopixelUpdate() {
    // Flash always takes priority
    if (_flashing) {
        updateFlash();
        return;
    }

    switch (_state) {
    case NEO_DETECTION_FLASH:
        // If flash was triggered via state (shouldn't normally happen)
        updateFlash();
        break;
    case NEO_HEARTBEAT_GLOW:
        updateHeartbeat();
        break;
    case NEO_IDLE_BREATHING:
    default:
        updateBreathing();
        break;
    }
}

} // namespace hal
