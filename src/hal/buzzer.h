#pragma once

#include <Arduino.h>
#include "pins.h"

// ============================================================================
// Non-blocking Buzzer HAL with named sound effects and priority system
// ============================================================================

namespace hal {

// Sound effect IDs
enum SoundEffect : uint8_t {
    SND_NONE = 0,
    // Boot jingles (low priority)
    SND_ZELDA_SECRET,       // Detector/Foxhunter/Selector boot
    SND_CROW_CALL,          // Flock-You boot
    SND_CLOSE_ENCOUNTERS,   // Sky Spy boot
    // Detection alerts (high priority)
    SND_THREE_BEEPS,        // Detector: new device / 30s re-detect
    SND_TWO_BEEPS,          // Detector: 3s re-detect
    SND_ASCENDING,          // Detector: ready to scan
    SND_CROW_ALARM,         // Flock-You: detection alert
    SND_CROW_HEARTBEAT,     // Flock-You: device in range
    SND_DRONE_DETECT,       // Sky Spy: 3 rapid beeps
    SND_DRONE_HEARTBEAT,    // Sky Spy: double pulse
    SND_FOX_FIRST,          // Foxhunter: 3 same-tone beeps (target acquired)
    // Special: foxhunter proximity (handled separately)
};

// Priority levels (higher number = higher priority)
enum SoundPriority : uint8_t {
    PRI_IDLE = 0,
    PRI_HEARTBEAT = 1,
    PRI_BOOT = 2,
    PRI_DETECTION = 3,
    PRI_PROXIMITY = 4,     // Foxhunter continuous beeping
};

void buzzerInit();
void buzzerUpdate();  // Call every loop iteration
void buzzerPlay(SoundEffect effect);
bool buzzerIsPlaying();
void buzzerStop();

// Foxhunter proximity mode: continuous RSSI-mapped beeping
void buzzerSetProximity(bool enabled, int rssi = -100);

// Global enable/disable (reads from NVS on init)
bool buzzerIsEnabled();
void buzzerSetEnabled(bool enabled);

} // namespace hal
