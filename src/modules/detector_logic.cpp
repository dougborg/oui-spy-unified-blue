#include "detector_logic.h"

#include <algorithm>
#include <cctype>

namespace detector_logic {

std::string normalizeMac(const std::string& mac) {
    std::string out;
    out.reserve(mac.size());
    for (char value : mac) {
        if (value == ' ')
            continue;
        if (value == '-')
            out.push_back(':');
        else
            out.push_back((char)std::tolower((unsigned char)value));
    }
    return out;
}

bool isValidMac(const std::string& mac) {
    std::string normalized = normalizeMac(mac);
    if (normalized.size() != 8 && normalized.size() != 17)
        return false;

    for (size_t index = 0; index < normalized.size(); ++index) {
        char value = normalized[index];
        if (index % 3 == 2) {
            if (value != ':')
                return false;
        } else if (!std::isxdigit((unsigned char)value)) {
            return false;
        }
    }
    return true;
}

bool matchesFilter(const std::string& deviceMac, const std::string& filterIdentifier,
                   bool isFullMac) {
    std::string normalizedDevice = normalizeMac(deviceMac);
    std::string normalizedFilter = normalizeMac(filterIdentifier);

    if (isFullMac)
        return normalizedDevice == normalizedFilter;

    return normalizedDevice.rfind(normalizedFilter, 0) == 0;
}

CooldownResult evaluateCooldown(bool inCooldown, uint32_t cooldownUntil, uint32_t lastSeen,
                                uint32_t now) {
    if (inCooldown && now < cooldownUntil)
        return {DetectionType::IN_COOLDOWN, 0};

    uint32_t sinceLast = now - lastSeen;

    if (sinceLast >= 30000)
        return {DetectionType::RE_30S, 10000};

    if (sinceLast >= 3000)
        return {DetectionType::RE_3S, 3000};

    return {DetectionType::TOO_RECENT, 0};
}

} // namespace detector_logic
