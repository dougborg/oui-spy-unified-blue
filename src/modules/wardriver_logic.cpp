#include "wardriver_logic.h"

namespace wardriver_logic {

const char* authModeToString(int authMode) {
    // Maps ESP32 wifi_auth_mode_t values to WiGLE-compatible strings
    switch (authMode) {
    case 0:
        return "[OPEN]";
    case 1:
        return "[WEP]";
    case 2:
        return "[WPA-PSK]";
    case 3:
        return "[WPA2-PSK]";
    case 4:
        return "[WPA/WPA2-PSK]";
    case 5:
        return "[WPA2-ENTERPRISE]";
    case 6:
        return "[WPA3-PSK]";
    case 7:
        return "[WPA2/WPA3-PSK]";
    case 8:
        return "[WAPI-PSK]";
    default:
        return "[UNKNOWN]";
    }
}

int channelToFreqMHz(int channel) {
    if (channel >= 1 && channel <= 13)
        return 2412 + (channel - 1) * 5;
    if (channel == 14)
        return 2484;
    return 0;
}

std::string sanitizeSSID(const char* ssid) {
    if (!ssid || ssid[0] == '\0')
        return "";

    std::string out;
    bool needsQuote = false;

    // Check if quoting is needed
    for (const char* p = ssid; *p; ++p) {
        if (*p == ',' || *p == '"' || *p == '\n' || *p == '\r') {
            needsQuote = true;
            break;
        }
    }

    if (!needsQuote)
        return std::string(ssid);

    out.push_back('"');
    for (const char* p = ssid; *p; ++p) {
        if (*p == '"')
            out.append("\"\""); // Escape double quotes by doubling
        else if (*p == '\n' || *p == '\r')
            out.push_back(' '); // Replace newlines with space
        else
            out.push_back(*p);
    }
    out.push_back('"');
    return out;
}

std::string formatWigleRow(const char* mac, const char* ssid, const char* authMode,
                           const char* type, int channel, int rssi, double lat, double lon,
                           float accuracy, const char* timestamp) {
    // WiGLE CSV format:
    // MAC,SSID,AuthMode,FirstSeen,Channel,RSSI,CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,Type
    std::string row;
    row.reserve(200);

    row.append(mac);
    row.push_back(',');
    row.append(sanitizeSSID(ssid));
    row.push_back(',');
    row.append(authMode);
    row.push_back(',');
    row.append(timestamp);
    row.push_back(',');

    // Channel
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", channel);
    row.append(buf);
    row.push_back(',');

    // RSSI
    snprintf(buf, sizeof(buf), "%d", rssi);
    row.append(buf);
    row.push_back(',');

    // Lat
    snprintf(buf, sizeof(buf), "%.8f", lat);
    row.append(buf);
    row.push_back(',');

    // Lon
    snprintf(buf, sizeof(buf), "%.8f", lon);
    row.append(buf);
    row.push_back(',');

    // Altitude (not available, use 0)
    row.append("0");
    row.push_back(',');

    // Accuracy
    snprintf(buf, sizeof(buf), "%.1f", (double)accuracy);
    row.append(buf);
    row.push_back(',');

    // Type
    row.append(type);

    return row;
}

uint32_t hashMAC(const char* mac) {
    // FNV-1a 32-bit hash
    uint32_t hash = 2166136261u;
    for (const char* p = mac; *p; ++p) {
        hash ^= (uint32_t)(unsigned char)*p;
        hash *= 16777619u;
    }
    // Ensure non-zero (0 is our empty sentinel)
    return hash == 0 ? 1 : hash;
}

bool dedupInsert(uint32_t* set, int capacity, uint32_t hash) {
    if (hash == 0)
        return false;
    uint32_t idx = hash % (uint32_t)capacity;
    for (int i = 0; i < capacity; ++i) {
        uint32_t pos = (idx + (uint32_t)i) % (uint32_t)capacity;
        if (set[pos] == 0) {
            set[pos] = hash;
            return true; // Inserted new
        }
        if (set[pos] == hash)
            return false; // Already present
    }
    return false; // Table full
}

bool dedupContains(const uint32_t* set, int capacity, uint32_t hash) {
    if (hash == 0)
        return false;
    uint32_t idx = hash % (uint32_t)capacity;
    for (int i = 0; i < capacity; ++i) {
        uint32_t pos = (idx + (uint32_t)i) % (uint32_t)capacity;
        if (set[pos] == 0)
            return false; // Empty slot = not found
        if (set[pos] == hash)
            return true;
    }
    return false; // Full table, not found
}

int dedupCount(const uint32_t* set, int capacity) {
    int count = 0;
    for (int i = 0; i < capacity; ++i) {
        if (set[i] != 0)
            ++count;
    }
    return count;
}

} // namespace wardriver_logic
