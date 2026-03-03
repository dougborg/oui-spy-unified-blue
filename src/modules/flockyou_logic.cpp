#include "flockyou_logic.h"

#include <cstdio>
#include <cstring>
#include <strings.h>

namespace flockyou_logic {

bool macMatchesPrefixes(const uint8_t* mac, const char* const* prefixes, size_t prefixCount) {
    if (!mac || !prefixes)
        return false;

    char prefix[9] = {0};
    std::snprintf(prefix, sizeof(prefix), "%02x:%02x:%02x", mac[0], mac[1], mac[2]);

    for (size_t index = 0; index < prefixCount; ++index) {
        if (::strncasecmp(prefix, prefixes[index], 8) == 0)
            return true;
    }
    return false;
}

bool nameMatchesPatterns(const char* name, const char* const* patterns, size_t patternCount) {
    if (!name || !name[0] || !patterns)
        return false;

    for (size_t index = 0; index < patternCount; ++index) {
        if (::strcasestr(name, patterns[index]))
            return true;
    }
    return false;
}

bool manufacturerMatches(uint16_t id, const uint16_t* knownIds, size_t idCount) {
    if (!knownIds)
        return false;

    for (size_t index = 0; index < idCount; ++index) {
        if (knownIds[index] == id)
            return true;
    }
    return false;
}

bool uuidEqualsAny(const char* uuid, const char* const* knownUuids, size_t uuidCount) {
    if (!uuid || !knownUuids)
        return false;

    for (size_t index = 0; index < uuidCount; ++index) {
        if (::strcasecmp(uuid, knownUuids[index]) == 0)
            return true;
    }
    return false;
}

const char* estimateRavenFirmware(bool hasNewGps, bool hasOldLocation, bool hasPowerService) {
    if (hasOldLocation && !hasNewGps)
        return "1.1.x";
    if (hasNewGps && !hasPowerService)
        return "1.2.x";
    if (hasNewGps && hasPowerService)
        return "1.3.x";
    return "?";
}

} // namespace flockyou_logic
