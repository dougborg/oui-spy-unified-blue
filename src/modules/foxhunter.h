#pragma once

#include "../hal/ble_mgr.h"
#include "module.h"

class FoxhunterModule : public IDetectionModule, public hal::BLEListener {
  public:
    const char* name() override {
        return "foxhunter";
    }
    void setup() override;
    void loop() override;
    void onBLEAdvertisement(NimBLEAdvertisedDevice* device) override;
    void registerRoutes(AsyncWebServer& server) override;
    bool isEnabled() override;
    void setEnabled(bool enabled) override;

  private:
    bool _enabled = true;
    String _targetMAC;
    bool _sessionFirstDetection = true;
    bool _targetDetected = false;
    int _currentRSSI = -100;
    unsigned long _lastTargetSeen = 0;
    unsigned long _lastRSSIPrint = 0;

    void loadConfig();
    void saveConfig();
};
