#pragma once

#include <Arduino.h>
#include <NimBLEAdvertisedDevice.h>
#include <ESPAsyncWebServer.h>
#include <esp_wifi.h>

// ============================================================================
// Detection Module Interface
// All detection engines implement this to run simultaneously.
// ============================================================================

class IDetectionModule {
public:
    virtual ~IDetectionModule() = default;

    virtual const char* name() = 0;
    virtual void setup() = 0;
    virtual void loop() = 0;

    // BLE advertisement dispatch (called for every scan result)
    virtual void onBLEAdvertisement(NimBLEAdvertisedDevice* device) = 0;

    // WiFi promiscuous frame dispatch (Sky Spy only; default no-op)
    virtual void onWiFiFrame(const uint8_t* payload, int len, int rssi) { (void)payload; (void)len; (void)rssi; }

    // Register module-specific web routes under /api/{module}/
    virtual void registerRoutes(AsyncWebServer& server) = 0;

    // Enable/disable
    virtual bool isEnabled() = 0;
    virtual void setEnabled(bool enabled) = 0;
};
