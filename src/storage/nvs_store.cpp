#include "nvs_store.h"
#include <Preferences.h>

namespace storage {

// ============================================================================
// Module enable/disable (namespace: "ouispy-mod")
// ============================================================================

bool getModuleEnabled(const char* name, bool defaultVal) {
    Preferences p;
    p.begin("ouispy-mod", true);
    bool val = p.getBool(name, defaultVal);
    p.end();
    return val;
}

void setModuleEnabled(const char* name, bool enabled) {
    Preferences p;
    p.begin("ouispy-mod", false);
    p.putBool(name, enabled);
    p.end();
}

// ============================================================================
// AP config (namespace: "ouispy-ap")
// ============================================================================

String getAPSSID(const char* defaultVal) {
    Preferences p;
    p.begin("ouispy-ap", true);
    String val = p.getString("ssid", defaultVal);
    p.end();
    return val;
}

String getAPPass(const char* defaultVal) {
    Preferences p;
    p.begin("ouispy-ap", true);
    String val = p.getString("pass", defaultVal);
    p.end();
    return val;
}

void setAPConfig(const String& ssid, const String& pass) {
    Preferences p;
    p.begin("ouispy-ap", false);
    p.putString("ssid", ssid);
    p.putString("pass", pass);
    p.end();
}

// ============================================================================
// Buzzer (namespace: "ouispy-bz")
// ============================================================================

bool getBuzzerEnabled(bool defaultVal) {
    Preferences p;
    p.begin("ouispy-bz", true);
    bool val = p.getBool("on", defaultVal);
    p.end();
    return val;
}

void setBuzzerEnabled(bool enabled) {
    Preferences p;
    p.begin("ouispy-bz", false);
    p.putBool("on", enabled);
    p.end();
}

// ============================================================================
// Detector filters (namespace: "ouispy")
// ============================================================================

int getDetFilterCount() {
    Preferences p;
    p.begin("ouispy", true);
    int val = p.getInt("filterCount", 0);
    p.end();
    return val;
}

void getDetFilter(int index, String& identifier, bool& isFullMAC, String& description) {
    Preferences p;
    p.begin("ouispy", true);
    identifier = p.getString(("id_" + String(index)).c_str(), "");
    isFullMAC = p.getBool(("mac_" + String(index)).c_str(), false);
    description = p.getString(("desc_" + String(index)).c_str(), "");
    p.end();
}

bool getDetBuzzerEnabled(bool defaultVal) {
    Preferences p;
    p.begin("ouispy", true);
    bool val = p.getBool("buzzerEnabled", defaultVal);
    p.end();
    return val;
}

bool getDetLedEnabled(bool defaultVal) {
    Preferences p;
    p.begin("ouispy", true);
    bool val = p.getBool("ledEnabled", defaultVal);
    p.end();
    return val;
}

void saveDetFilters(int count, const String* ids, const bool* fullMACs, const String* descs,
                    bool buzzerEnabled, bool ledEnabled) {
    Preferences p;
    p.begin("ouispy", false);
    p.putInt("filterCount", count);
    p.putBool("buzzerEnabled", buzzerEnabled);
    p.putBool("ledEnabled", ledEnabled);
    for (int i = 0; i < count; i++) {
        p.putString(("id_" + String(i)).c_str(), ids[i]);
        p.putBool(("mac_" + String(i)).c_str(), fullMACs[i]);
        p.putString(("desc_" + String(i)).c_str(), descs[i]);
    }
    p.end();
}

// ============================================================================
// Detector aliases (namespace: "ouispy")
// ============================================================================

int getDetAliasCount() {
    Preferences p;
    p.begin("ouispy", true);
    int val = p.getInt("aliasCount", 0);
    p.end();
    return val;
}

void getDetAlias(int index, String& macAddress, String& alias) {
    Preferences p;
    p.begin("ouispy", true);
    macAddress = p.getString(("alias_mac_" + String(index)).c_str(), "");
    alias = p.getString(("alias_name_" + String(index)).c_str(), "");
    p.end();
}

void saveDetAliases(int count, const String* macs, const String* aliases) {
    Preferences p;
    p.begin("ouispy", false);
    p.putInt("aliasCount", count);
    for (int i = 0; i < count; i++) {
        p.putString(("alias_mac_" + String(i)).c_str(), macs[i]);
        p.putString(("alias_name_" + String(i)).c_str(), aliases[i]);
    }
    p.end();
}

// ============================================================================
// Detector devices (namespace: "ouispy-dev")
// ============================================================================

int getDetDeviceCount() {
    Preferences p;
    p.begin("ouispy-dev", true);
    int val = p.getInt("count", 0);
    p.end();
    return val;
}

void getDetDevice(int index, String& mac, int& rssi, unsigned long& lastSeen, String& filter) {
    Preferences p;
    p.begin("ouispy-dev", true);
    mac = p.getString(("dm_" + String(index)).c_str(), "");
    rssi = p.getInt(("dr_" + String(index)).c_str(), 0);
    lastSeen = p.getULong(("dl_" + String(index)).c_str(), 0);
    filter = p.getString(("df_" + String(index)).c_str(), "");
    p.end();
}

void saveDetDevices(int count, const String* macs, const int* rssis, const unsigned long* lastSeens,
                    const String* filters) {
    Preferences p;
    p.begin("ouispy-dev", false);
    p.putInt("count", count);
    for (int i = 0; i < count; i++) {
        p.putString(("dm_" + String(i)).c_str(), macs[i]);
        p.putInt(("dr_" + String(i)).c_str(), rssis[i]);
        p.putULong(("dl_" + String(i)).c_str(), lastSeens[i]);
        p.putString(("df_" + String(i)).c_str(), filters[i]);
    }
    p.end();
}

void clearDetDevices() {
    Preferences p;
    p.begin("ouispy-dev", false);
    p.clear();
    p.end();
}

// ============================================================================
// Foxhunter target (namespace: "tracker")
// ============================================================================

String getFoxTargetMAC(const char* defaultVal) {
    Preferences p;
    p.begin("tracker", true);
    String val = p.getString("targetMAC", defaultVal);
    p.end();
    return val;
}

void setFoxTargetMAC(const String& mac) {
    Preferences p;
    p.begin("tracker", false);
    p.putString("targetMAC", mac);
    p.end();
}

// ============================================================================
// Wardriver filters (namespace: "ouispy-wd")
// ============================================================================

int getWdFilterCount() {
    Preferences p;
    p.begin("ouispy-wd", true);
    int val = p.getInt("filterCount", 0);
    p.end();
    return val;
}

void getWdFilter(int index, String& identifier, bool& isFullMAC, String& description) {
    Preferences p;
    p.begin("ouispy-wd", true);
    identifier = p.getString(("id_" + String(index)).c_str(), "");
    isFullMAC = p.getBool(("mac_" + String(index)).c_str(), false);
    description = p.getString(("desc_" + String(index)).c_str(), "");
    p.end();
}

void saveWdFilters(int count, const String* ids, const bool* fullMACs, const String* descs) {
    Preferences p;
    p.begin("ouispy-wd", false);
    p.putInt("filterCount", count);
    for (int i = 0; i < count; i++) {
        p.putString(("id_" + String(i)).c_str(), ids[i]);
        p.putBool(("mac_" + String(i)).c_str(), fullMACs[i]);
        p.putString(("desc_" + String(i)).c_str(), descs[i]);
    }
    p.end();
}

// ============================================================================
// Wardriver aliases (namespace: "ouispy-wd")
// ============================================================================

int getWdAliasCount() {
    Preferences p;
    p.begin("ouispy-wd", true);
    int val = p.getInt("aliasCount", 0);
    p.end();
    return val;
}

void getWdAlias(int index, String& macAddress, String& alias) {
    Preferences p;
    p.begin("ouispy-wd", true);
    macAddress = p.getString(("amac_" + String(index)).c_str(), "");
    alias = p.getString(("aname_" + String(index)).c_str(), "");
    p.end();
}

void saveWdAliases(int count, const String* macs, const String* aliases) {
    Preferences p;
    p.begin("ouispy-wd", false);
    p.putInt("aliasCount", count);
    for (int i = 0; i < count; i++) {
        p.putString(("amac_" + String(i)).c_str(), macs[i]);
        p.putString(("aname_" + String(i)).c_str(), aliases[i]);
    }
    p.end();
}

// ============================================================================
// Wardriver devices (namespace: "ouispy-wdd")
// ============================================================================

int getWdDeviceCount() {
    Preferences p;
    p.begin("ouispy-wdd", true);
    int val = p.getInt("count", 0);
    p.end();
    return val;
}

void getWdDevice(int index, String& mac, int& rssi, unsigned long& lastSeen, String& filter) {
    Preferences p;
    p.begin("ouispy-wdd", true);
    mac = p.getString(("wm_" + String(index)).c_str(), "");
    rssi = p.getInt(("wr_" + String(index)).c_str(), 0);
    lastSeen = p.getULong(("wl_" + String(index)).c_str(), 0);
    filter = p.getString(("wf_" + String(index)).c_str(), "");
    p.end();
}

void saveWdDevices(int count, const String* macs, const int* rssis, const unsigned long* lastSeens,
                   const String* filters) {
    Preferences p;
    p.begin("ouispy-wdd", false);
    p.putInt("count", count);
    for (int i = 0; i < count; i++) {
        p.putString(("wm_" + String(i)).c_str(), macs[i]);
        p.putInt(("wr_" + String(i)).c_str(), rssis[i]);
        p.putULong(("wl_" + String(i)).c_str(), lastSeens[i]);
        p.putString(("wf_" + String(i)).c_str(), filters[i]);
    }
    p.end();
}

void clearWdDevices() {
    Preferences p;
    p.begin("ouispy-wdd", false);
    p.clear();
    p.end();
}

// ============================================================================
// Wardriver WiGLE config (namespace: "ouispy-wdc")
// ============================================================================

String getWdApiName(const char* defaultVal) {
    Preferences p;
    p.begin("ouispy-wdc", true);
    String val = p.getString("apiName", defaultVal);
    p.end();
    return val;
}

String getWdApiToken(const char* defaultVal) {
    Preferences p;
    p.begin("ouispy-wdc", true);
    String val = p.getString("apiToken", defaultVal);
    p.end();
    return val;
}

void setWdApiConfig(const String& name, const String& token) {
    Preferences p;
    p.begin("ouispy-wdc", false);
    p.putString("apiName", name);
    p.putString("apiToken", token);
    p.end();
}

// ============================================================================
// Sky Spy scan mode boot flag (namespace: "ouispy-boot")
// One-shot flag: set before reboot, cleared immediately on scan boot.
// ============================================================================

bool getSkyScanRequested() {
    Preferences p;
    p.begin("ouispy-boot", true);
    bool val = p.getBool("scan", false);
    p.end();
    return val;
}

void setSkyScanRequested(bool requested) {
    Preferences p;
    p.begin("ouispy-boot", false);
    p.putBool("scan", requested);
    p.end();
}

void clearSkyScanRequested() {
    Preferences p;
    p.begin("ouispy-boot", false);
    p.remove("scan");
    p.end();
}

} // namespace storage
