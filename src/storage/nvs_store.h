#pragma once

#include <Arduino.h>

// ============================================================================
// Typed NVS accessor for all OUI-SPY namespaces
// Centralizes all Preferences access — no more magic strings in module code.
// ============================================================================

namespace storage {

// --- Module enable/disable (namespace: "ouispy-mod") ---
bool getModuleEnabled(const char* name, bool defaultVal = true);
void setModuleEnabled(const char* name, bool enabled);

// --- AP config (namespace: "ouispy-ap") ---
String getAPSSID(const char* defaultVal = "oui-spy");
String getAPPass(const char* defaultVal = "ouispy123");
void setAPConfig(const String& ssid, const String& pass);

// --- Buzzer (namespace: "ouispy-bz") ---
bool getBuzzerEnabled(bool defaultVal = true);
void setBuzzerEnabled(bool enabled);

// --- Detector filters (namespace: "ouispy") ---
int getDetFilterCount();
void getDetFilter(int index, String& identifier, bool& isFullMAC, String& description);
bool getDetBuzzerEnabled(bool defaultVal = true);
bool getDetLedEnabled(bool defaultVal = true);
void saveDetFilters(int count, const String* ids, const bool* fullMACs, const String* descs,
                    bool buzzerEnabled, bool ledEnabled);

// --- Detector aliases (namespace: "ouispy") ---
int getDetAliasCount();
void getDetAlias(int index, String& macAddress, String& alias);
void saveDetAliases(int count, const String* macs, const String* aliases);

// --- Detector devices (namespace: "ouispy-dev") ---
int getDetDeviceCount();
void getDetDevice(int index, String& mac, int& rssi, unsigned long& lastSeen, String& filter);
void saveDetDevices(int count, const String* macs, const int* rssis, const unsigned long* lastSeens,
                    const String* filters);
void clearDetDevices();

// --- Foxhunter target (namespace: "tracker") ---
String getFoxTargetMAC(const char* defaultVal = "");
void setFoxTargetMAC(const String& mac);

} // namespace storage
