#pragma once

#include <Arduino.h>

// ============================================================================
// Captive Portal DNS Server
// Resolves ALL DNS queries to the AP IP address (192.168.4.1).
// Enables captive portal auto-popup on iOS/Android/Windows.
// ============================================================================

namespace hal {

void dnsServerStart();   // Start listening on UDP port 53
void dnsServerLoop();    // Non-blocking poll for incoming queries

} // namespace hal
