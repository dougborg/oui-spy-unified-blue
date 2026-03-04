#include "dns_server.h"
#include <DNSServer.h>
#include <WiFi.h>

namespace hal {

static DNSServer _dns;
static bool _dnsRunning = false;

void dnsInit() {
    _dns.start(53, "*", WiFi.softAPIP());
    _dnsRunning = true;
    Serial.printf("[HAL] DNS captive portal started → %s\n", WiFi.softAPIP().toString().c_str());
}

void dnsUpdate() {
    if (_dnsRunning) {
        _dns.processNextRequest();
    }
}

void dnsStop() {
    if (_dnsRunning) {
        _dns.stop();
        _dnsRunning = false;
        Serial.println("[HAL] DNS captive portal stopped");
    }
}

} // namespace hal
