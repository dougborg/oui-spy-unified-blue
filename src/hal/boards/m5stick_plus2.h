#pragma once

// ============================================================================
// M5StickC Plus 2 — Compact wearable form factor
// MCU: ESP32-PICO-V3-02 | Flash: 8MB | PSRAM: 0
// Display: 1.14" ST7789V2 135x240 SPI (deferred)
// ============================================================================

#define BOARD_NAME "M5StickC Plus 2"

// Capabilities
#define HAS_BUZZER 1
#define HAS_NEOPIXEL 0
#define HAS_APA102 0
#define HAS_LED 1
#define HAS_GPS_UART 0
#define HAS_DISPLAY 1
#define HAS_PSRAM 0
#define LED_ACTIVE_LOW 0

// Pin definitions
#define BUZZER_PIN 2
#define LED_PIN 19
#define BOOT_BUTTON_PIN 37
#define POWER_HOLD_PIN 4
