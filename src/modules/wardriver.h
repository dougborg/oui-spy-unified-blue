#pragma once

#include "../hal/ble_mgr.h"
#include "module.h"
#include <vector>

#define WD_DEDUP_CAPACITY 2048
#define WD_RECENT_SIZE 50
#define WD_CSV_PATH "/wigle.csv"

struct WDRecentSighting {
    String mac;
    String ssid;
    String type; // "WIFI" or "BLE"
    int rssi;
    int channel;
    unsigned long timestamp;
};

struct WDTargetFilter {
    String identifier;
    bool isFullMAC;
    String description;
};

struct WDDeviceInfo {
    String macAddress;
    int rssi;
    unsigned long firstSeen;
    unsigned long lastSeen;
    bool inCooldown;
    unsigned long cooldownUntil;
    String filterDescription;
};

struct WDDeviceAlias {
    String macAddress;
    String alias;
};

class WardriverModule : public IModule, public hal::BLEListener {
  public:
    const char* name() override {
        return "wardriver";
    }
    void setup() override;
    void loop() override;
    void registerRoutes(httpd_handle_t https, httpd_handle_t http) override;
    bool isEnabled() override;
    void setEnabled(bool enabled) override;

    // hal::BLEListener
    void onBLEAdvertisement(NimBLEAdvertisedDevice* device) override;

    // Accessors for route handlers
    bool sessionActive() const {
        return _sessionActive;
    }
    int wifiCount() const {
        return _wifiCount;
    }
    int bleCount() const {
        return _bleCount;
    }
    int uniqueCount() const;
    // Thread-safe copies (for route handlers called from web server task)
    void getFilters(std::vector<WDTargetFilter>& out) const;
    void getWatchlistDevices(std::vector<WDDeviceInfo>& out) const;
    int filterCount() const;

    // Recent sightings ring buffer (newest first)
    void getRecentSightings(std::vector<WDRecentSighting>& out) const;

    // Public operations for route handlers
    void startSession();
    void stopSession();
    void clearData();
    size_t csvFileSize() const;
    bool hasCsvFile() const;

    // Filter management
    void parseFilters(const String& ouis, const String& macs);
    void saveFilters();
    void clearFilters();

    // Alias management
    String getAlias(const String& mac);
    void setAlias(const String& mac, const String& alias);
    void saveAliases();

    // Watchlist device management
    void clearWatchlistDevices();

    // WiGLE config
    String wigleApiName() const {
        return _wigleApiName;
    }
    String wigleApiToken() const {
        return _wigleApiToken;
    }
    void setWigleConfig(const String& apiName, const String& apiToken);

  private:
    bool _enabled = false;

    // Session state
    bool _sessionActive = false;
    int _wifiCount = 0;
    int _bleCount = 0;

    // MAC dedup hash set (8KB static, no heap alloc)
    uint32_t _dedupSet[WD_DEDUP_CAPACITY] = {};

    // Recent sightings ring buffer
    WDRecentSighting _recent[WD_RECENT_SIZE] = {};
    int _recentHead = 0;
    int _recentCount = 0;

    // Watchlist
    std::vector<WDTargetFilter> _filters;
    std::vector<WDDeviceInfo> _wlDevices;
    std::vector<WDDeviceAlias> _aliases;

    // WiGLE API config
    String _wigleApiName;
    String _wigleApiToken;

    // Thread safety (BLE callback vs web/loop)
    SemaphoreHandle_t _mutex = nullptr;

    // Timers
    unsigned long _lastScanStart = 0;
    unsigned long _lastSave = 0;
    bool _scanPending = false;

    // Internal helpers
    void addRecent(const String& mac, const String& ssid, const String& type, int rssi, int channel);
    void appendCsvRow(const String& row);
    void writeCsvHeader();
    bool matchesFilter(const String& deviceMAC, String& matchedDesc);
    void checkWatchlist(const String& mac, int rssi, const String& type);

    // NVS persistence
    void loadFilters();
    void loadAliases();
    void saveDevices();
    void loadDevices();
    void loadWigleConfig();
};
