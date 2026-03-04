#pragma once

#include "../hal/ble_mgr.h"
#include "module.h"
#include "opendroneid.h"
#include <cstdint>

#define SS_MAX_UAVS 8

struct SSUavData {
    uint8_t mac[6];
    int rssi;
    uint32_t lastSeen;
    char opId[ODID_ID_SIZE + 1];
    char uavId[ODID_ID_SIZE + 1];
    double latD;
    double longD;
    double baseLatD;
    double baseLongD;
    int altitudeMsl;
    int heightAgl;
    int speed;
    int heading;
    int flag;
};

class SkySpyModule : public IModule, public hal::BLEListener {
  public:
    const char* name() override {
        return "skyspy";
    }
    void setup() override;
    void loop() override;
    void registerRoutes(AsyncWebServer& server) override;
    bool isEnabled() override;
    void setEnabled(bool enabled) override;

    // hal::BLEListener
    void onBLEAdvertisement(NimBLEAdvertisedDevice* device) override;

    // Called from static WiFi callback
    void handleWiFiFrame(const uint8_t* payload, int length, int rssi);

    // Public accessors for route handlers
    SemaphoreHandle_t uavMutex() const {
        return _mutex;
    }
    const SSUavData& uavAt(int index) const {
        return _uavs[index];
    }
    bool deviceInRange() const {
        return _deviceInRange;
    }

  private:
    bool _enabled = true;
    SSUavData _uavs[SS_MAX_UAVS] = {};
    ODID_UAS_Data _uasData = {};
    SemaphoreHandle_t _mutex = nullptr;
    bool _deviceInRange = false;
    unsigned long _lastHeartbeat = 0;
    unsigned long _lastStatus = 0;

    SSUavData* nextUav(uint8_t* mac);
    void sendJSON(const SSUavData* uav);
    void extractFromODID(SSUavData* uav);
    void triggerDetection();
};

// Static WiFi promiscuous callback (cannot be a member function)
void skyspyWiFiCallback(void* buf, wifi_promiscuous_pkt_type_t type);
