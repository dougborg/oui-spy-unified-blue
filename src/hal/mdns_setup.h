#pragma once

#include <Arduino.h>

// ============================================================================
// mDNS Setup — advertise ouispy.local on the AP network
// ============================================================================

namespace hal {

void mdnsInit(); // Start mDNS with hostname "ouispy" + HTTP/HTTPS service records

} // namespace hal
