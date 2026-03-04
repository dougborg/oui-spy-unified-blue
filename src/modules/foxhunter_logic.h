#pragma once

#include <cstdint>

namespace foxhunter_logic {

// Proximity state machine outcomes
enum class ProxEvent {
    NONE,        // No state change
    TARGET_LOST, // Target timed out
};

// Result of processing a proximity tick (loop iteration)
struct ProxTickResult {
    ProxEvent event;
    bool shouldBeep;       // true if proximity beep should sound
    bool shouldPrintRSSI;  // true if RSSI should be printed
};

// Evaluate the proximity state machine during loop().
// targetDetected: is the target currently tracked?
// now: current timestamp (millis)
// lastTargetSeen: when target was last seen
// lastRSSIPrint: when RSSI was last printed
// targetLostTimeoutMs: how long before target is considered lost (default 5000)
// rssiPrintIntervalMs: interval between RSSI prints (default 2000)
ProxTickResult evaluateProxTick(bool targetDetected, uint32_t now, uint32_t lastTargetSeen,
                                uint32_t lastRSSIPrint, uint32_t targetLostTimeoutMs = 5000,
                                uint32_t rssiPrintIntervalMs = 2000);

// Result of processing a BLE advertisement for target tracking
enum class TargetMatchEvent {
    NO_MATCH,            // MAC doesn't match target
    UPDATE_EXISTING,     // Target already tracked, just update RSSI/time
    FIRST_ACQUISITION,   // First time seeing target this session
    REACQUIRED,          // Target was lost, now seen again
};

// Evaluate whether a scanned MAC matches the target and what event that represents.
TargetMatchEvent evaluateTargetMatch(const char* scannedMac, const char* targetMac,
                                     bool targetDetected, bool sessionFirstDetection);

} // namespace foxhunter_logic
