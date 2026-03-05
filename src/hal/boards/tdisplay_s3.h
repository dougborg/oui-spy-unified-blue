#pragma once

// ============================================================================
// LilyGo T-Display-S3 — 1.9" LCD form factor
// MCU: ESP32-S3 | Flash: 16MB | PSRAM: 8MB
// Display: 1.9" ST7789 170x320 8-bit parallel (deferred)
// ============================================================================

#define BOARD_NAME "T-Display-S3"

// Capabilities
#define HAS_BUZZER 0
#define HAS_NEOPIXEL 0
#define HAS_APA102 0
#define HAS_LED 0
#define HAS_GPS_UART 0
#define HAS_DISPLAY 1
#define HAS_PSRAM 1
#define LED_ACTIVE_LOW 0

// Pin definitions
#define BOOT_BUTTON_PIN 0
