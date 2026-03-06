#pragma once

#include "../hal/ble_mgr.h"
#include "module.h"
#include <esp_http_server.h>
#include <freertos/portmacro.h>
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
    void registerRoutes(httpd_handle_t https, httpd_handle_t http) override;
    bool isEnabled() override;
    void setEnabled(bool enabled) override;

    // hal::BLEListener
    void onBLEAdvertisement(const NimBLEAdvertisedDevice* device) override;

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
    void parseFilters(const String& ouis, const String& macs);
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

    // Serial output from BLE callback (guarded by _matchMux)
    // Uses fixed-size char buffers so strlcpy inside critical section avoids heap ops
    portMUX_TYPE _matchMux = portMUX_INITIALIZER_UNLOCKED;
    volatile bool _newMatch = false;
    char _matchMAC[18] = {};    // "XX:XX:XX:XX:XX:XX\0"
    int _matchRSSI = 0;
    char _matchFilter[64] = {};
    char _matchType[8] = {};    // "NEW", "RE-3s", "RE-30s"

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
