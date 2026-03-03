#pragma once

// ============================================================================
// OUI-SPY Hardware Pin Definitions (Seeed XIAO ESP32-S3)
// Single source of truth - all modules use these.
// ============================================================================

#define BUZZER_PIN 3      // GPIO3 (D2) - PWM capable
#define LED_PIN 21        // GPIO21 - Built-in orange LED (inverted: LOW=ON)
#define NEOPIXEL_PIN 4    // GPIO4 (D3) - WS2812 NeoPixel
#define BOOT_BUTTON_PIN 0 // GPIO0 - BOOT button (active LOW)
#define GPS_RX_PIN 44     // D7 - ESP32 RX <- GPS TX
#define GPS_TX_PIN 43     // D6 - ESP32 TX -> GPS RX
