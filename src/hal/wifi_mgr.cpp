#include "wifi_mgr.h"
#include "mac_util.h"

namespace hal {

static bool _apRunning = false;
static bool _promiscuousEnabled = false;

void wifiInit(const String& ssid, const String& password) {
    // Clean WiFi state
    WiFi.mode(WIFI_AP_STA);
    delay(100);
    esp_wifi_restore();
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true, true);
    delay(100);

    // Set AP+STA mode
    WiFi.persistent(false);
    WiFi.mode(WIFI_AP_STA);
    delay(200);

    // Start AP
    bool ok;
    if (password.length() >= 8) {
        ok = WiFi.softAP(ssid.c_str(), password.c_str());
    } else {
        ok = WiFi.softAP(ssid.c_str());
    }

    // Randomize MAC after AP is started (requires esp_wifi_start)
    randomizeMAC();

    Serial.printf("[HAL] WiFi AP '%s': %s\n", ssid.c_str(), ok ? "OK" : "FAILED");
    Serial.printf("[HAL] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    _apRunning = ok;
}

void wifiUpdate() {
    // Reserved for future use
}

void wifiEnablePromiscuous(WiFiFrameCallback cb, uint8_t channel) {
    if (_promiscuousEnabled) return;
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(cb);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    _promiscuousEnabled = true;
    Serial.printf("[HAL] WiFi promiscuous mode ON (channel %d)\n", channel);
}

void wifiDisablePromiscuous() {
    if (!_promiscuousEnabled) return;
    esp_wifi_set_promiscuous(false);
    _promiscuousEnabled = false;
    Serial.println("[HAL] WiFi promiscuous mode OFF");
}

IPAddress wifiGetAPIP() {
    return WiFi.softAPIP();
}

} // namespace hal
