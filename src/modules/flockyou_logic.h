#pragma once

#include <cstddef>
#include <cstdint>

namespace flockyou_logic {

bool macMatchesPrefixes(const uint8_t* mac, const char* const* prefixes, size_t prefixCount);
bool nameMatchesPatterns(const char* name, const char* const* patterns, size_t patternCount);
bool manufacturerMatches(uint16_t id, const uint16_t* knownIds, size_t idCount);
bool uuidEqualsAny(const char* uuid, const char* const* knownUuids, size_t uuidCount);
const char* estimateRavenFirmware(bool hasNewGps, bool hasOldLocation, bool hasPowerService);

} // namespace flockyou_logic
