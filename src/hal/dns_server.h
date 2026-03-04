#pragma once

// ============================================================================
// DNS Captive Portal — redirects all DNS queries to the AP IP
// ============================================================================

namespace hal {

void dnsInit();   // Start DNS on port 53, redirect "*" to softAPIP()
void dnsUpdate(); // Call in loop() — processNextRequest()
void dnsStop();

} // namespace hal
