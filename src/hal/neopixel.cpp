#include "neopixel.h"
#include "neopixel_logic.h"

namespace hal {

#define NEOPIXEL_BRIGHTNESS 50
#define NEOPIXEL_DETECTION_BRIGHTNESS 200

// ============================================================================
// Backend selection
// ============================================================================

#if HAS_NEOPIXEL
// --- WS2812 NeoPixel backend ---
static Adafruit_NeoPixel _strip(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
#define STRIP _strip
#elif HAS_APA102
// --- APA102 DotStar backend ---
static Adafruit_DotStar _strip(APA102_NUM_LEDS, APA102_DATA_PIN, APA102_CLK_PIN, DOTSTAR_BRG);
#define STRIP _strip
#endif

// ============================================================================
// Common state (used by NeoPixel/APA102/LED backends)
// ============================================================================

static NeoPixelState _state = NEO_IDLE_BREATHING;
static uint16_t _idleHue = 300;
static uint16_t _heartbeatHue = 300;

static bool _flashing = false;
static unsigned long _flashStart = 0;
static uint16_t _flashHues[3] = {240, 300, 270};

static float _breatheBrightness = 0.0;
static bool _breatheIncreasing = true;
static unsigned long _breatheLastUpdate = 0;

static float _scanBrightness = 0.0;
static bool _scanIncreasing = true;
static unsigned long _scanLastUpdate = 0;

// ============================================================================
// HSV to RGB — standalone (no strip dependency)
// ============================================================================

uint32_t hsvToRgb(uint16_t h, uint8_t s, uint8_t v) {
    uint8_t r, g, b;
    if (s == 0) {
        r = g = b = v;
    } else {
        uint8_t h8 = (uint8_t)((uint32_t)h * 255 / 360);
        uint8_t region = h8 / 43;
        uint8_t remainder = (h8 - (region * 43)) * 6;
        uint8_t p = (v * (255 - s)) >> 8;
        uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
        uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
        switch (region) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
        }
    }
#if HAS_NEOPIXEL || HAS_APA102
    return STRIP.Color(r, g, b);
#else
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
#endif
}

// ============================================================================
// NeoPixel / APA102 backend implementation
// ============================================================================

#if HAS_NEOPIXEL || HAS_APA102

#if HAS_NEOPIXEL
Adafruit_NeoPixel& neopixelStrip() { return _strip; }
#else
Adafruit_DotStar& neopixelStrip() { return _strip; }
#endif

void neopixelInit() {
    STRIP.begin();
    STRIP.setBrightness(NEOPIXEL_BRIGHTNESS);
    STRIP.clear();
    STRIP.show();
    Serial.println("[HAL] NeoPixel initialized");
}

void neopixelSetColor(uint8_t r, uint8_t g, uint8_t b) {
    STRIP.setPixelColor(0, STRIP.Color(r, g, b));
    STRIP.show();
}

void neopixelOff() {
    STRIP.clear();
    STRIP.show();
}

static void updateBreathing() {
    if (millis() - _breatheLastUpdate < 20) return;
    _breatheLastUpdate = millis();

    neopixel_logic::BreathingState state{_breatheBrightness, _breatheIncreasing};
    state = neopixel_logic::nextBreathing(state);
    _breatheBrightness = state.brightness;
    _breatheIncreasing = state.increasing;

    uint32_t color = hsvToRgb(_idleHue, 255, (uint8_t)(NEOPIXEL_BRIGHTNESS * _breatheBrightness));
    STRIP.setPixelColor(0, color);
    STRIP.show();
}

static void updateFlash() {
    unsigned long elapsed = millis() - _flashStart;
    auto frame = neopixel_logic::computeFlashFrame(elapsed, NEOPIXEL_DETECTION_BRIGHTNESS,
                                                   (NEOPIXEL_BRIGHTNESS / 4));
    if (!frame.active) {
        _flashing = false;
        return;
    }
    STRIP.setPixelColor(0, hsvToRgb(_flashHues[frame.frameIndex], 255, frame.value));
    STRIP.show();
}

static void updateHeartbeat() {
    STRIP.setPixelColor(0, hsvToRgb(_heartbeatHue, 255, 30));
    STRIP.show();
}

static void updateScanModeCycle() {
    if (millis() - _scanLastUpdate < 25) return;
    _scanLastUpdate = millis();

    if (_scanIncreasing) {
        _scanBrightness += 0.02f;
        if (_scanBrightness >= 1.0f) { _scanBrightness = 1.0f; _scanIncreasing = false; }
    } else {
        _scanBrightness -= 0.02f;
        if (_scanBrightness <= 0.15f) { _scanBrightness = 0.15f; _scanIncreasing = true; }
    }

    uint16_t hue = 180 + (uint16_t)(60.0f * _scanBrightness);
    uint8_t val = (uint8_t)(NEOPIXEL_BRIGHTNESS * _scanBrightness);
    STRIP.setPixelColor(0, hsvToRgb(hue, 255, val));
    STRIP.show();
}

// ============================================================================
// Plain LED fallback backend
// ============================================================================

#elif HAS_LED

// Plain LED fallback — use fully qualified names since we're inside namespace hal
// and led.h also defines in namespace hal

static inline void plainLedInit() {
    pinMode(LED_PIN, OUTPUT);
#if LED_ACTIVE_LOW
    digitalWrite(LED_PIN, HIGH);
#else
    digitalWrite(LED_PIN, LOW);
#endif
}

static inline void plainLedOn() {
#if LED_ACTIVE_LOW
    digitalWrite(LED_PIN, LOW);
#else
    digitalWrite(LED_PIN, HIGH);
#endif
}

static inline void plainLedOff() {
#if LED_ACTIVE_LOW
    digitalWrite(LED_PIN, HIGH);
#else
    digitalWrite(LED_PIN, LOW);
#endif
}

void neopixelInit() {
    plainLedInit();
    Serial.println("[HAL] NeoPixel: using plain LED fallback");
}

void neopixelSetColor(uint8_t r, uint8_t g, uint8_t b) {
    if (r > 0 || g > 0 || b > 0) plainLedOn(); else plainLedOff();
}

void neopixelOff() { plainLedOff(); }

static unsigned long _ledBlinkLast = 0;
static bool _ledBlinkState = false;

static void updateBreathing() {
    if (millis() - _ledBlinkLast < 1000) return;
    _ledBlinkLast = millis();
    _ledBlinkState = !_ledBlinkState;
    if (_ledBlinkState) plainLedOn(); else plainLedOff();
}

static void updateFlash() {
    unsigned long elapsed = millis() - _flashStart;
    if (elapsed > 750) { _flashing = false; plainLedOff(); return; }
    if ((elapsed / 100) % 2 == 0) plainLedOn(); else plainLedOff();
}

static void updateHeartbeat() { plainLedOn(); }

static void updateScanModeCycle() {
    if (millis() - _scanLastUpdate < 250) return;
    _scanLastUpdate = millis();
    _scanBrightness = _scanBrightness > 0.5f ? 0.0f : 1.0f;
    if (_scanBrightness > 0.5f) plainLedOn(); else plainLedOff();
}

// ============================================================================
// No-op backend (boards with no LED/NeoPixel — display-only boards)
// ============================================================================

#else

void neopixelInit() {
    Serial.println("[HAL] NeoPixel: no LED hardware (display-only board)");
}
void neopixelSetColor(uint8_t, uint8_t, uint8_t) {}
void neopixelOff() {}
static void updateBreathing() {}
static void updateFlash() { _flashing = false; }
static void updateHeartbeat() {}
static void updateScanModeCycle() {}

#endif // backend selection

// ============================================================================
// Common functions (all backends)
// ============================================================================

void neopixelSetIdleHue(uint16_t hue) { _idleHue = hue; }

void neopixelSetState(NeoPixelState state, uint16_t hue) {
    _state = state;
    if (state == NEO_HEARTBEAT_GLOW) _heartbeatHue = hue;
}

void neopixelFlash(uint16_t hue1, uint16_t hue2, uint16_t hue3) {
    _flashing = true;
    _flashStart = millis();
    _flashHues[0] = hue1;
    _flashHues[1] = hue2;
    _flashHues[2] = hue3;
}

void neopixelStartScanAnimation() { _state = NEO_SCAN_MODE_CYCLE; }

void neopixelUpdate() {
    if (_flashing) { updateFlash(); return; }

    switch (_state) {
    case NEO_SCAN_MODE_CYCLE: updateScanModeCycle(); break;
    case NEO_DETECTION_FLASH: updateFlash(); break;
    case NEO_HEARTBEAT_GLOW: updateHeartbeat(); break;
    case NEO_IDLE_BREATHING:
    default: updateBreathing(); break;
    }
}

} // namespace hal
