#pragma once

#include <cstdint>
#include <cstring>
#include <string>

namespace wardriver_logic {

// wifi_auth_mode_t value → WiGLE auth string
const char* authModeToString(int authMode);

// 2.4GHz channel number → frequency in MHz (returns 0 for invalid)
int channelToFreqMHz(int channel);

// CSV-safe SSID escaping (quotes commas, quotes, newlines)
std::string sanitizeSSID(const char* ssid);

// Format one WiGLE CSV data row
// type: "WIFI" or "BLE"
std::string formatWigleRow(const char* mac, const char* ssid, const char* authMode, const char* type,
                           int channel, int rssi, double lat, double lon, float accuracy,
                           const char* timestamp);

// FNV-1a hash of a MAC string for dedup
uint32_t hashMAC(const char* mac);

// Open-addressing hash set operations on a fixed uint32_t[] array
// Returns true if inserted (was not present)
bool dedupInsert(uint32_t* set, int capacity, uint32_t hash);

// Returns true if the hash is already in the set
bool dedupContains(const uint32_t* set, int capacity, uint32_t hash);

// Count non-zero entries in the set
int dedupCount(const uint32_t* set, int capacity);

} // namespace wardriver_logic
