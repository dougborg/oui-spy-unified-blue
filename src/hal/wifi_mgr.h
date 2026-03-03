#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>

// ============================================================================
// Unified WiFi Manager
// WIFI_AP_STA mode: AP serves web dashboard, STA enables promiscuous for Sky Spy
// ============================================================================

namespace hal {

// WiFi promiscuous frame callback type (for Sky Spy)
using WiFiFrameCallback = void (*)(void* buf, wifi_promiscuous_pkt_type_t type);

void wifiInit(const String& ssid, const String& password);
void wifiUpdate(); // Currently no-op, reserved for future use

// Enable/disable promiscuous mode for drone detection
void wifiEnablePromiscuous(WiFiFrameCallback cb, uint8_t channel = 6);
void wifiDisablePromiscuous();

// Get AP IP
IPAddress wifiGetAPIP();

} // namespace hal
