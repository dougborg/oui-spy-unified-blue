#include "detector.h"
#include "../hal/buzzer.h"
#include "../hal/neopixel.h"
#include "../hal/led.h"
#include <Preferences.h>
#include <vector>
#include <algorithm>

// ============================================================================
// Detector Module: BLE device scanning with OUI/MAC watchlists
// Refactored from raw/detector.cpp — all HAL code removed, uses shared services
// ============================================================================

// --- Data structures (preserved from original) ---

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

static std::vector<DetDeviceInfo> detDevices;
static std::vector<DetTargetFilter> detFilters;
static std::vector<DetDeviceAlias> detAliases;
static bool detBuzzerEnabled = true;
static bool detLedEnabled = true;
static bool detEnabled = true;

// Serial output from BLE callback
static volatile bool detNewMatch = false;
static String detMatchMAC;
static int detMatchRSSI;
static String detMatchFilter;
static String detMatchType;

// NVS auto-save timer
static unsigned long detLastSave = 0;

// ============================================================================
// MAC Utility Functions
// ============================================================================

static void detNormMAC(String& mac) {
    mac.toLowerCase();
    mac.replace("-", ":");
    mac.replace(" ", "");
}

static bool detIsValidMAC(const String& mac) {
    String n = mac;
    detNormMAC(n);
    if (n.length() != 8 && n.length() != 17) return false;
    for (int i = 0; i < (int)n.length(); i++) {
        char c = n.charAt(i);
        if (i % 3 == 2) { if (c != ':') return false; }
        else { if (!isxdigit(c)) return false; }
    }
    return true;
}

static bool detMatchesFilter(const String& deviceMAC, String& matchedDesc) {
    String norm = deviceMAC;
    detNormMAC(norm);
    for (const auto& f : detFilters) {
        String fid = f.identifier;
        detNormMAC(fid);
        if (f.isFullMAC) {
            if (norm.equals(fid)) { matchedDesc = f.description; return true; }
        } else {
            if (norm.startsWith(fid)) { matchedDesc = f.description; return true; }
        }
    }
    return false;
}

// ============================================================================
// NVS Persistence
// ============================================================================

static void detSaveFilters() {
    Preferences p;
    p.begin("ouispy", false);
    p.putInt("filterCount", detFilters.size());
    p.putBool("buzzerEnabled", detBuzzerEnabled);
    p.putBool("ledEnabled", detLedEnabled);
    for (int i = 0; i < (int)detFilters.size(); i++) {
        p.putString(("id_" + String(i)).c_str(), detFilters[i].identifier);
        p.putBool(("mac_" + String(i)).c_str(), detFilters[i].isFullMAC);
        p.putString(("desc_" + String(i)).c_str(), detFilters[i].description);
    }
    p.end();
}

static void detLoadFilters() {
    Preferences p;
    p.begin("ouispy", true);
    int count = p.getInt("filterCount", 0);
    detBuzzerEnabled = p.getBool("buzzerEnabled", true);
    detLedEnabled = p.getBool("ledEnabled", true);
    detFilters.clear();
    for (int i = 0; i < count; i++) {
        DetTargetFilter f;
        f.identifier = p.getString(("id_" + String(i)).c_str(), "");
        f.isFullMAC = p.getBool(("mac_" + String(i)).c_str(), false);
        f.description = p.getString(("desc_" + String(i)).c_str(), "");
        if (f.identifier.length() > 0) detFilters.push_back(f);
    }
    p.end();
}

static void detSaveAliases() {
    Preferences p;
    p.begin("ouispy", false);
    p.putInt("aliasCount", detAliases.size());
    for (int i = 0; i < (int)detAliases.size(); i++) {
        p.putString(("alias_mac_" + String(i)).c_str(), detAliases[i].macAddress);
        p.putString(("alias_name_" + String(i)).c_str(), detAliases[i].alias);
    }
    p.end();
}

static void detLoadAliases() {
    Preferences p;
    p.begin("ouispy", true);
    int count = p.getInt("aliasCount", 0);
    detAliases.clear();
    for (int i = 0; i < count; i++) {
        DetDeviceAlias a;
        a.macAddress = p.getString(("alias_mac_" + String(i)).c_str(), "");
        a.alias = p.getString(("alias_name_" + String(i)).c_str(), "");
        if (a.macAddress.length() > 0 && a.alias.length() > 0) detAliases.push_back(a);
    }
    p.end();
}

static String detGetAlias(const String& mac) {
    String norm = mac;
    detNormMAC(norm);
    for (const auto& a : detAliases) {
        String n = a.macAddress;
        detNormMAC(n);
        if (n.equals(norm)) return a.alias;
    }
    return "";
}

static void detSetAlias(const String& mac, const String& alias) {
    String norm = mac;
    detNormMAC(norm);
    for (auto& a : detAliases) {
        String n = a.macAddress;
        detNormMAC(n);
        if (n.equals(norm)) {
            if (alias.length() == 0) {
                detAliases.erase(std::remove_if(detAliases.begin(), detAliases.end(),
                    [&](const DetDeviceAlias& da) {
                        String nm = da.macAddress; detNormMAC(nm);
                        return nm.equals(norm);
                    }), detAliases.end());
            } else {
                a.alias = alias;
            }
            return;
        }
    }
    if (alias.length() > 0) {
        detAliases.push_back({norm, alias});
    }
}

static void detSaveDevices() {
    Preferences p;
    p.begin("ouispy-dev", false);
    int count = min((int)detDevices.size(), 100);
    p.putInt("count", count);
    for (int i = 0; i < count; i++) {
        p.putString(("dm_" + String(i)).c_str(), detDevices[i].macAddress);
        p.putInt(("dr_" + String(i)).c_str(), detDevices[i].rssi);
        p.putULong(("dl_" + String(i)).c_str(), detDevices[i].lastSeen);
        p.putString(("df_" + String(i)).c_str(), detDevices[i].filterDescription);
    }
    p.end();
}

static void detLoadDevices() {
    Preferences p;
    p.begin("ouispy-dev", true);
    int count = p.getInt("count", 0);
    detDevices.clear();
    for (int i = 0; i < count; i++) {
        DetDeviceInfo d;
        d.macAddress = p.getString(("dm_" + String(i)).c_str(), "");
        d.rssi = p.getInt(("dr_" + String(i)).c_str(), 0);
        d.lastSeen = p.getULong(("dl_" + String(i)).c_str(), 0);
        d.filterDescription = p.getString(("df_" + String(i)).c_str(), "");
        d.firstSeen = d.lastSeen;
        d.inCooldown = false;
        d.cooldownUntil = 0;
        if (d.macAddress.length() > 0) detDevices.push_back(d);
    }
    p.end();
}

// ============================================================================
// Module Implementation
// ============================================================================

void DetectorModule::setup() {
    detLoadFilters();
    detLoadAliases();
    detLoadDevices();
    Serial.printf("[DETECTOR] Loaded %d filters, %d aliases, %d devices\n",
                  (int)detFilters.size(), (int)detAliases.size(), (int)detDevices.size());
}

void DetectorModule::loop() {
    if (!_enabled) return;

    // Output JSON for new matches
    if (detNewMatch) {
        String alias = detGetAlias(detMatchMAC);
        Serial.printf("{\"module\":\"detector\",\"mac\":\"%s\",\"alias\":\"%s\",\"rssi\":%d,\"type\":\"%s\"}\n",
                      detMatchMAC.c_str(), alias.c_str(), detMatchRSSI, detMatchType.c_str());
        detNewMatch = false;
    }

    // Auto-save every 10s
    if (millis() - detLastSave >= 10000) {
        detSaveDevices();
        detLastSave = millis();
    }
}

void DetectorModule::onBLEAdvertisement(NimBLEAdvertisedDevice* device) {
    if (!_enabled) return;
    if (detFilters.empty()) return;

    String mac = device->getAddress().toString().c_str();
    int rssi = device->getRSSI();
    unsigned long now = millis();

    String matchedDesc;
    if (!detMatchesFilter(mac, matchedDesc)) return;

    // Check existing devices
    for (auto& dev : detDevices) {
        if (dev.macAddress == mac) {
            if (dev.inCooldown && now < dev.cooldownUntil) return;
            if (dev.inCooldown && now >= dev.cooldownUntil) dev.inCooldown = false;

            unsigned long sinceLast = now - dev.lastSeen;
            if (sinceLast >= 30000) {
                detMatchMAC = mac; detMatchRSSI = rssi;
                detMatchFilter = matchedDesc; detMatchType = "RE-30s";
                detNewMatch = true;
                hal::buzzerPlay(hal::SND_THREE_BEEPS);
                hal::neopixelFlash(240, 300, 270);
                dev.inCooldown = true;
                dev.cooldownUntil = now + 10000;
            } else if (sinceLast >= 3000) {
                detMatchMAC = mac; detMatchRSSI = rssi;
                detMatchFilter = matchedDesc; detMatchType = "RE-3s";
                detNewMatch = true;
                hal::buzzerPlay(hal::SND_TWO_BEEPS);
                hal::neopixelFlash(240, 300, 270);
                dev.inCooldown = true;
                dev.cooldownUntil = now + 3000;
            }
            dev.lastSeen = now;
            dev.rssi = rssi;
            return;
        }
    }

    // New device
    DetDeviceInfo newDev;
    newDev.macAddress = mac;
    newDev.rssi = rssi;
    newDev.firstSeen = now;
    newDev.lastSeen = now;
    newDev.inCooldown = true;
    newDev.cooldownUntil = now + 3000;
    newDev.filterDescription = matchedDesc;
    detDevices.push_back(newDev);

    detMatchMAC = mac; detMatchRSSI = rssi;
    detMatchFilter = matchedDesc; detMatchType = "NEW";
    detNewMatch = true;
    hal::buzzerPlay(hal::SND_THREE_BEEPS);
    hal::neopixelFlash(240, 300, 270);
}

// ============================================================================
// Web Routes: /api/detector/*
// ============================================================================

static void detParseFilters(AsyncWebServerRequest* request) {
    detFilters.clear();
    // OUI entries
    if (request->hasParam("ouis", true)) {
        String data = request->getParam("ouis", true)->value();
        data.trim();
        if (data.length() > 0) {
            int start = 0, end = data.indexOf('\n');
            while (start < (int)data.length()) {
                String line;
                if (end == -1) { line = data.substring(start); start = data.length(); }
                else { line = data.substring(start, end); start = end + 1; end = data.indexOf('\n', start); }
                line.trim(); line.replace("\r", "");
                if (line.length() > 0 && detIsValidMAC(line)) {
                    detFilters.push_back({line, false, "OUI: " + line});
                }
            }
        }
    }
    // MAC entries
    if (request->hasParam("macs", true)) {
        String data = request->getParam("macs", true)->value();
        data.trim();
        if (data.length() > 0) {
            int start = 0, end = data.indexOf('\n');
            while (start < (int)data.length()) {
                String line;
                if (end == -1) { line = data.substring(start); start = data.length(); }
                else { line = data.substring(start, end); start = end + 1; end = data.indexOf('\n', start); }
                line.trim(); line.replace("\r", "");
                if (line.length() > 0 && detIsValidMAC(line)) {
                    detFilters.push_back({line, true, "MAC: " + line});
                }
            }
        }
    }
}

void DetectorModule::registerRoutes(AsyncWebServer& server) {
    // Get filters
    server.on("/api/detector/filters", HTTP_GET, [](AsyncWebServerRequest* r) {
        String json = "[";
        for (int i = 0; i < (int)detFilters.size(); i++) {
            if (i > 0) json += ",";
            json += "{\"id\":\"" + detFilters[i].identifier + "\",\"full\":" +
                    (detFilters[i].isFullMAC ? "true" : "false") +
                    ",\"desc\":\"" + detFilters[i].description + "\"}";
        }
        json += "]";
        r->send(200, "application/json", json);
    });

    // Save filters
    server.on("/api/detector/filters", HTTP_POST, [](AsyncWebServerRequest* r) {
        detParseFilters(r);
        // Buzzer/LED toggles
        detBuzzerEnabled = r->hasParam("buzzerEnabled", true);
        detLedEnabled = r->hasParam("ledEnabled", true);
        detSaveFilters();
        r->send(200, "application/json", "{\"saved\":" + String(detFilters.size()) + "}");
    });

    // Get detected devices
    server.on("/api/detector/devices", HTTP_GET, [](AsyncWebServerRequest* r) {
        String json = "{\"devices\":[";
        unsigned long now = millis();
        for (int i = 0; i < (int)detDevices.size(); i++) {
            if (i > 0) json += ",";
            String alias = detGetAlias(detDevices[i].macAddress);
            unsigned long ts = (now >= detDevices[i].lastSeen) ? (now - detDevices[i].lastSeen) : 0;
            json += "{\"mac\":\"" + detDevices[i].macAddress + "\",\"rssi\":" + String(detDevices[i].rssi) +
                    ",\"filter\":\"" + detDevices[i].filterDescription +
                    "\",\"alias\":\"" + alias + "\",\"timeSince\":" + String(ts) + "}";
        }
        json += "]}";
        r->send(200, "application/json", json);
    });

    // Save alias
    server.on("/api/detector/alias", HTTP_POST, [](AsyncWebServerRequest* r) {
        if (r->hasParam("mac", true) && r->hasParam("alias", true)) {
            detSetAlias(r->getParam("mac", true)->value(), r->getParam("alias", true)->value());
            detSaveAliases();
            r->send(200, "application/json", "{\"success\":true}");
        } else {
            r->send(400, "application/json", "{\"error\":\"missing params\"}");
        }
    });

    // Clear device history
    server.on("/api/detector/clear-devices", HTTP_POST, [](AsyncWebServerRequest* r) {
        detDevices.clear();
        Preferences p; p.begin("ouispy-dev", false); p.clear(); p.end();
        r->send(200, "application/json", "{\"success\":true}");
    });

    // Clear filters
    server.on("/api/detector/clear-filters", HTTP_POST, [](AsyncWebServerRequest* r) {
        detFilters.clear();
        detSaveFilters();
        r->send(200, "application/json", "{\"success\":true}");
    });

    Serial.println("[DETECTOR] Web routes registered");
}

bool DetectorModule::isEnabled() { return _enabled; }
void DetectorModule::setEnabled(bool enabled) {
    _enabled = enabled;
    detEnabled = enabled;
}
