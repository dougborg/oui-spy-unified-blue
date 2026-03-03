#pragma once

#include <Arduino.h>
#include <esp_wifi.h>

// ============================================================================
// MAC Address Randomization (shared across all modules)
// ============================================================================

namespace hal {

inline void randomizeMAC() {
    uint8_t mac[6];
    uint32_t r1 = esp_random();
    uint32_t r2 = esp_random();
    mac[0] = (r1 >> 0) & 0xFF;
    mac[1] = (r1 >> 8) & 0xFF;
    mac[2] = (r1 >> 16) & 0xFF;
    mac[3] = (r1 >> 24) & 0xFF;
    mac[4] = (r2 >> 0) & 0xFF;
    mac[5] = (r2 >> 8) & 0xFF;
    mac[0] = (mac[0] | 0x02) & 0xFE; // locally administered, unicast
    esp_wifi_set_mac(WIFI_IF_AP, mac);
    Serial.printf("[HAL] Randomized MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

} // namespace hal
