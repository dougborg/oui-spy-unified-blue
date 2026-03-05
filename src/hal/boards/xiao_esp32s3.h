#pragma once

// ============================================================================
// Seeed XIAO ESP32-S3 — Original OUI-SPY board
// MCU: ESP32-S3 | Flash: 8MB | PSRAM: 8MB (OPI)
// ============================================================================

#define BOARD_NAME "XIAO ESP32-S3"

// Capabilities
#define HAS_BUZZER 1
#define HAS_NEOPIXEL 1
#define HAS_APA102 0
#define HAS_LED 1
#define HAS_GPS_UART 1
#define HAS_DISPLAY 0
#define HAS_PSRAM 1
#define LED_ACTIVE_LOW 1

// Pin definitions
#define BUZZER_PIN 3
#define LED_PIN 21
#define NEOPIXEL_PIN 4
#define BOOT_BUTTON_PIN 0
#define GPS_RX_PIN 44
#define GPS_TX_PIN 43
