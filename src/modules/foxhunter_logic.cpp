#include "foxhunter_logic.h"

#include <cstring>
#include <strings.h>

namespace foxhunter_logic {

ProxTickResult evaluateProxTick(bool targetDetected, uint32_t now, uint32_t lastTargetSeen,
                                uint32_t lastRSSIPrint, uint32_t targetLostTimeoutMs,
                                uint32_t rssiPrintIntervalMs) {
    ProxTickResult result = {ProxEvent::NONE, false, false};

    if (!targetDetected)
        return result;

    uint32_t elapsed = now - lastTargetSeen;

    if (elapsed < targetLostTimeoutMs) {
        // Target still in range — beep and maybe print RSSI
        result.shouldBeep = true;
        if (now - lastRSSIPrint >= rssiPrintIntervalMs)
            result.shouldPrintRSSI = true;
    } else {
        // Target lost
        result.event = ProxEvent::TARGET_LOST;
    }

    return result;
}

TargetMatchEvent evaluateTargetMatch(const char* scannedMac, const char* targetMac,
                                     bool targetDetected, bool sessionFirstDetection) {
    if (!scannedMac || !targetMac || !targetMac[0])
        return TargetMatchEvent::NO_MATCH;

    if (::strcasecmp(scannedMac, targetMac) != 0)
        return TargetMatchEvent::NO_MATCH;

    if (targetDetected)
        return TargetMatchEvent::UPDATE_EXISTING;

    if (sessionFirstDetection)
        return TargetMatchEvent::FIRST_ACQUISITION;

    return TargetMatchEvent::REACQUIRED;
}

} // namespace foxhunter_logic
