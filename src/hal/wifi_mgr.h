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

// Normal mode: AP+STA for web dashboard + BLE modules
void wifiInit(const String& ssid, const String& password);
void wifiUpdate(); // Currently no-op, reserved for future use

// Scan mode: STA-only, no AP, safe to set channel
void wifiInitScanMode();
bool wifiIsScanMode();

// Enable promiscuous in scan mode (STA-only, sets channel explicitly)
void wifiEnablePromiscuousScanMode(WiFiFrameCallback cb, uint8_t channel = 6);

// Enable/disable promiscuous mode (normal AP mode — uses AP channel)
void wifiEnablePromiscuous(WiFiFrameCallback cb, uint8_t channel = 6);
void wifiDisablePromiscuous();

// WiFi network scan (for wardriving)
bool wifiStartNetworkScan();  // Start async scan, returns false if already scanning
int wifiGetScanComplete();    // -1=in progress, -2=not started, >=0=count
void wifiScanCleanup();       // Delete scan results

// Get AP IP
IPAddress wifiGetAPIP();

} // namespace hal
