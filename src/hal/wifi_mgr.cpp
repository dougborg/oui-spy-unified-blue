#include "wifi_mgr.h"
#include <dhcpserver/dhcpserver.h>
#include <dhcpserver/dhcpserver_options.h>

namespace hal {

static bool _apRunning = false;
static bool _promiscuousEnabled = false;
static bool _scanMode = false;
static bool _scanInProgress = false;

void wifiInit(const String& ssid, const String& password) {
    // AP-only mode for normal operation (STA not needed — scan mode uses separate init)
    WiFi.mode(WIFI_AP);
    delay(200);

    // Explicit IP/subnet config
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1),
                       IPAddress(255, 255, 255, 0));

    // Start AP on channel 1
    bool ok;
    if (password.length() >= 8) {
        ok = WiFi.softAP(ssid.c_str(), password.c_str(), 1, 0, 4);
        Serial.printf("[HAL] WiFi AP '%s' ch1 (WPA2): %s\n", ssid.c_str(), ok ? "OK" : "FAILED");
    } else {
        ok = WiFi.softAP(ssid.c_str(), NULL, 1, 0, 4);
        Serial.printf("[HAL] WiFi AP '%s' ch1 (OPEN): %s\n", ssid.c_str(), ok ? "OK" : "FAILED");
    }

    delay(500); // Let DHCP server settle

    // Configure DHCP server to advertise our AP IP as DNS server
    // This enables the captive portal DNS to intercept all queries
    IPAddress apIP = WiFi.softAPIP();
    dhcps_offer_t offer = OFFER_DNS;
    dhcps_set_option_info(6, &offer, sizeof(offer));

    Serial.printf("[HAL] AP IP: %s\n", apIP.toString().c_str());
    _apRunning = ok;
    _scanMode = false;
}

void wifiInitScanMode() {
    // STA-only mode — no AP, safe to set channel explicitly
    WiFi.mode(WIFI_STA);
    delay(100);
    esp_wifi_restore();
    WiFi.disconnect(true, true);
    delay(100);

    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    delay(200);

    _apRunning = false;
    _scanMode = true;
    Serial.println("[HAL] WiFi STA-only scan mode initialized");
}

bool wifiIsScanMode() {
    return _scanMode;
}

void wifiUpdate() {
    // Reserved for future use
}

void wifiEnablePromiscuousScanMode(WiFiFrameCallback cb, uint8_t channel) {
    if (_promiscuousEnabled)
        return;

    // In STA-only mode we can safely set the channel
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    wifi_promiscuous_filter_t filter = {.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT};
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous_rx_cb(cb);
    esp_wifi_set_promiscuous(true);
    _promiscuousEnabled = true;
    Serial.printf("[HAL] WiFi promiscuous scan mode ON (channel %d)\n", channel);
}

void wifiEnablePromiscuous(WiFiFrameCallback cb, uint8_t channel) {
    if (_promiscuousEnabled)
        return;

    // Set promiscuous filter to only receive management frames
    wifi_promiscuous_filter_t filter = {.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT};
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous_rx_cb(cb);
    esp_wifi_set_promiscuous(true);
    // Do NOT call esp_wifi_set_channel() — in AP mode the AP subsystem owns the
    // channel.  Promiscuous mode receives on whatever channel the radio is tuned to.
    _promiscuousEnabled = true;
    Serial.printf("[HAL] WiFi promiscuous mode ON (AP channel)\n");
}

void wifiDisablePromiscuous() {
    if (!_promiscuousEnabled)
        return;
    esp_wifi_set_promiscuous(false);
    _promiscuousEnabled = false;
    Serial.println("[HAL] WiFi promiscuous mode OFF");
}

bool wifiStartNetworkScan() {
    if (_scanInProgress)
        return false;

    _scanInProgress = true;
    WiFi.scanNetworks(true); // async=true
    return true;
}

int wifiGetScanComplete() {
    if (!_scanInProgress)
        return -2;
    return WiFi.scanComplete();
}

void wifiScanCleanup() {
    WiFi.scanDelete();
    _scanInProgress = false;
}

IPAddress wifiGetAPIP() {
    return WiFi.softAPIP();
}

} // namespace hal
