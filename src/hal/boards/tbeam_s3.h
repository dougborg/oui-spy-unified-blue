#pragma once

// ============================================================================
// LilyGo T-Beam S3 Supreme — GPS + LoRa board
// MCU: ESP32-S3 | Flash: 8MB | PSRAM: 8MB
// Display: 1.3" SH1106 128x64 I2C OLED (deferred)
// GPS: L76K or U-blox on UART1
// ============================================================================

#define BOARD_NAME "T-Beam S3 Supreme"

// Capabilities
#define HAS_BUZZER 0
#define HAS_NEOPIXEL 0
#define HAS_APA102 0
#define HAS_LED 0
#define HAS_GPS_UART 1
#define HAS_DISPLAY 1
#define HAS_PSRAM 1
#define LED_ACTIVE_LOW 0

// Pin definitions
#define BOOT_BUTTON_PIN 0
#define GPS_RX_PIN 34
#define GPS_TX_PIN 33
