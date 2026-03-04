#pragma once

#include <cstdint>
#include <string>

namespace detector_logic {

std::string normalizeMac(const std::string& mac);
bool isValidMac(const std::string& mac);
bool matchesFilter(const std::string& deviceMac, const std::string& filterIdentifier,
                   bool isFullMac);

// Cooldown / re-detection classification
enum class DetectionType {
    IN_COOLDOWN, // Device is in cooldown, ignore
    RE_30S,      // Re-detected after >=30s absence — strong re-detection
    RE_3S,       // Re-detected after >=3s absence — mild re-detection
    TOO_RECENT,  // Seen less than 3s ago, no event
};

struct CooldownResult {
    DetectionType type;
    uint32_t newCooldownMs; // How long to set cooldown after this event (0 = no change)
};

// Evaluate whether a re-detected device should trigger an alert.
// inCooldown: device is currently rate-limited
// cooldownUntil: timestamp when cooldown expires
// lastSeen: when device was last seen
// now: current timestamp
CooldownResult evaluateCooldown(bool inCooldown, uint32_t cooldownUntil, uint32_t lastSeen,
                                uint32_t now);

} // namespace detector_logic
