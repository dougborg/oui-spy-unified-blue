#pragma once

#include "../hal/ble_mgr.h"
#include "module.h"

class FoxhunterModule : public IModule, public hal::BLEListener {
  public:
    const char* name() override {
        return "foxhunter";
    }
    void setup() override;
    void loop() override;
    void registerRoutes(AsyncWebServer& server) override;
    bool isEnabled() override;
    void setEnabled(bool enabled) override;

    // hal::BLEListener
    void onBLEAdvertisement(NimBLEAdvertisedDevice* device) override;

    // Public accessors for route handlers
    const String& targetMAC() const {
        return _targetMAC;
    }
    bool targetDetected() const {
        return _targetDetected;
    }
    int currentRSSI() const {
        return _currentRSSI;
    }
    unsigned long lastTargetSeen() const {
        return _lastTargetSeen;
    }

    // Public operations for route handlers
    void setTarget(const String& mac);
    void clearTarget();

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
