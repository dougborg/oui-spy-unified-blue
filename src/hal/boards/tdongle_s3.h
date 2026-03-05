#pragma once

// ============================================================================
// LilyGo T-Dongle-S3 — USB dongle form factor
// MCU: ESP32-S3 | Flash: 16MB | PSRAM: 8MB
// Display: 0.96" ST7735 80x160 SPI (deferred)
// ============================================================================

#define BOARD_NAME "T-Dongle-S3"

// Capabilities
#define HAS_BUZZER 0
#define HAS_NEOPIXEL 0
#define HAS_APA102 1
#define HAS_LED 0
#define HAS_GPS_UART 0
#define HAS_DISPLAY 1
#define HAS_PSRAM 1
#define LED_ACTIVE_LOW 0

// APA102 (DotStar) LED
#define APA102_DATA_PIN 40
#define APA102_CLK_PIN 39
#define APA102_NUM_LEDS 1

// Pin definitions
#define BOOT_BUTTON_PIN 0
