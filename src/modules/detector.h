#pragma once

#include "../hal/ble_mgr.h"
#include "module.h"
#include <vector>

struct DetDeviceInfo {
    String macAddress;
    int rssi;
    unsigned long firstSeen;
    unsigned long lastSeen;
    bool inCooldown;
    unsigned long cooldownUntil;
    String filterDescription;
};

struct DetTargetFilter {
    String identifier;
    bool isFullMAC;
    String description;
};

struct DetDeviceAlias {
    String macAddress;
    String alias;
};

class DetectorModule : public IModule, public hal::BLEListener {
  public:
    const char* name() override {
        return "detector";
    }
    void setup() override;
    void loop() override;
    void registerRoutes(AsyncWebServer& server) override;
    bool isEnabled() override;
    void setEnabled(bool enabled) override;

    // hal::BLEListener
    void onBLEAdvertisement(NimBLEAdvertisedDevice* device) override;

    // Accessors for route handlers
    const std::vector<DetTargetFilter>& filters() const {
        return _filters;
    }
    const std::vector<DetDeviceInfo>& devices() const {
        return _devices;
    }
    bool buzzerEnabled() const {
        return _buzzerEnabled;
    }
    bool ledEnabled() const {
        return _ledEnabled;
    }

    // Public operations for route handlers
    void parseFiltersFromRequest(AsyncWebServerRequest* request);
    void setBuzzerEnabled(bool enabled) {
        _buzzerEnabled = enabled;
    }
    void setLedEnabled(bool enabled) {
        _ledEnabled = enabled;
    }
    void saveFilters();
    void saveAliases();
    String getAlias(const String& mac);
    void setAlias(const String& mac, const String& alias);
    void clearDevices();
    void clearFilters();

  private:
    bool _enabled = true;
    std::vector<DetDeviceInfo> _devices;
    std::vector<DetTargetFilter> _filters;
    std::vector<DetDeviceAlias> _aliases;
    bool _buzzerEnabled = true;
    bool _ledEnabled = true;

    // Serial output from BLE callback
    volatile bool _newMatch = false;
    String _matchMAC;
    int _matchRSSI = 0;
    String _matchFilter;
    String _matchType;

    // NVS auto-save timer
    unsigned long _lastSave = 0;

    // MAC helpers
    void normMAC(String& mac);
    bool isValidMAC(const String& mac);
    bool matchesFilter(const String& deviceMAC, String& matchedDesc);

    // NVS persistence
    void loadFilters();
    void loadAliases();
    void saveDevices();
    void loadDevices();
};
