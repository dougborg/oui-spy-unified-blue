#pragma once

#include <cstddef>
#include <cstdint>

namespace flockyou_logic {

bool macMatchesPrefixes(const uint8_t* mac, const char* const* prefixes, size_t prefixCount);
bool nameMatchesPatterns(const char* name, const char* const* patterns, size_t patternCount);
bool manufacturerMatches(uint16_t id, const uint16_t* knownIds, size_t idCount);
bool uuidEqualsAny(const char* uuid, const char* const* knownUuids, size_t uuidCount);
const char* estimateRavenFirmware(bool hasNewGps, bool hasOldLocation, bool hasPowerService);

// Detection dedup: find existing detection index by MAC (case-insensitive).
// Returns index if found, -1 if not.
int findDetectionByMac(const char* const* macs, int count, const char* searchMac);

// Check if a device has been out of range (not seen within timeoutMs).
bool isOutOfRange(uint32_t lastDetTime, uint32_t now, uint32_t timeoutMs = 30000);

// Determine if heartbeat should fire based on interval.
bool shouldHeartbeat(uint32_t lastHeartbeat, uint32_t now, uint32_t intervalMs = 10000);

// Sanitize a device name character (replace quotes/backslashes with underscore).
char sanitizeNameChar(char c);

} // namespace flockyou_logic
