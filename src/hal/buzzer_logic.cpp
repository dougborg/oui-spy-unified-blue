#include "buzzer_logic.h"

namespace {

int mapRange(int value, int inMin, int inMax, int outMin, int outMax) {
    return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

} // namespace

namespace hal::buzzer_logic {

int calcProximityIntervalMs(int rssi) {
    if (rssi >= -35)
        return mapRange(rssi, -35, -25, 25, 10);
    if (rssi >= -45)
        return mapRange(rssi, -45, -35, 50, 25);
    if (rssi >= -55)
        return mapRange(rssi, -55, -45, 100, 50);
    if (rssi >= -65)
        return mapRange(rssi, -65, -55, 200, 100);
    if (rssi >= -75)
        return mapRange(rssi, -75, -65, 500, 200);
    if (rssi >= -85)
        return mapRange(rssi, -85, -75, 1000, 500);
    return 3000;
}

} // namespace hal::buzzer_logic
