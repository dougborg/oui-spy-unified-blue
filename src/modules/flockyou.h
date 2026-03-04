#pragma once

#include "../hal/ble_mgr.h"
#include "module.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#define FY_MAX_DETECTIONS 200

struct FYDetection {
    char mac[18];
    char name[48];
    int rssi;
    char method[24];
    unsigned long firstSeen;
    unsigned long lastSeen;
    int count;
    bool isRaven;
    char ravenFW[16];
    double gpsLat;
    double gpsLon;
    float gpsAcc;
    bool hasGPS;
};

class FlockyouModule : public IDetectionModule, public hal::BLEListener {
  public:
    const char* name() override {
        return "flockyou";
    }
    void setup() override;
    void loop() override;
    void onBLEAdvertisement(NimBLEAdvertisedDevice* device) override;
    void registerRoutes(AsyncWebServer& server) override;
    bool isEnabled() override;
    void setEnabled(bool enabled) override;

    // Accessors for route handlers (mutex must be held by caller)
    const FYDetection* detections() const {
        return _det;
    }
    int detectionCount() const {
        return _detCount;
    }
    SemaphoreHandle_t mutex() const {
        return _mutex;
    }
    bool fsReady() const {
        return _fsReady;
    }

    // Public operations for route handlers
    void writeJSON(AsyncResponseStream* resp);
    void writeKML(AsyncResponseStream* resp);
    void writeCSV(AsyncResponseStream* resp);
    void writePatterns(AsyncResponseStream* resp);
    void clearDetections();
    void saveSession();
    void serveHistory(AsyncWebServerRequest* r);
    void serveHistoryDownload(AsyncWebServerRequest* r);

  private:
    bool _enabled = true;
    FYDetection _det[FY_MAX_DETECTIONS] = {};
    int _detCount = 0;
    SemaphoreHandle_t _mutex = nullptr;
    bool _triggered = false;
    bool _deviceInRange = false;
    unsigned long _lastDetTime = 0;
    unsigned long _lastHB = 0;

    // Session persistence
    unsigned long _lastSave = 0;
    int _lastSaveCount = 0;
    bool _fsReady = false;

    // Detection helpers
    int addDetection(const char* mac, const char* name, int rssi, const char* method,
                     bool isRaven = false, const char* ravenFW = "");
    void attachGPS(FYDetection& d);

    // Session persistence
    void promotePrevSession();
};
