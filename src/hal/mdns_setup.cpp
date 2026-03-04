#include "mdns_setup.h"
#include <ESPmDNS.h>

namespace hal {

void mdnsInit() {
    if (MDNS.begin("ouispy")) {
        MDNS.addService("http", "tcp", 80);
        MDNS.addService("https", "tcp", 443);
        Serial.println("[HAL] mDNS started: ouispy.local");
    } else {
        Serial.println("[HAL] mDNS failed to start");
    }
}

} // namespace hal
