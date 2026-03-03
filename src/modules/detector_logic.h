#pragma once

#include <string>

namespace detector_logic {

std::string normalizeMac(const std::string& mac);
bool isValidMac(const std::string& mac);
bool matchesFilter(const std::string& deviceMac, const std::string& filterIdentifier,
                   bool isFullMac);

} // namespace detector_logic
