#pragma once

#include <Arduino.h>

// ============================================================================
// Notification abstraction — maps semantic events to buzzer + neopixel actions
// ============================================================================

namespace hal {

enum NotifyEvent : uint8_t {
    // Detector
    NOTIFY_DET_NEW_DEVICE, // New device detected (3 beeps + cyan/mag/purple flash)
    NOTIFY_DET_RE_3S,      // Re-detection after 3s (2 beeps + flash)
    NOTIFY_DET_RE_30S,     // Re-detection after 30s (3 beeps + flash)
    // Flockyou
    NOTIFY_FY_ALERT,     // Flipper detection alert (crow alarm + red/pink flash)
    NOTIFY_FY_HEARTBEAT, // Device in range heartbeat (crow heartbeat + glow)
    NOTIFY_FY_IDLE,      // Device out of range (return to idle breathing)
    // Skyspy
    NOTIFY_SS_DRONE,     // Drone detected (3 rapid beeps)
    NOTIFY_SS_HEARTBEAT, // Drone in range heartbeat (double pulse)
    // Foxhunter
    NOTIFY_FOX_ACQUIRED, // Target first acquired (3 same-tone beeps)
    // Wardriver
    NOTIFY_WD_NEW_DEVICE,    // Watchlist new (3 beeps + cyan/mag/purple flash)
    NOTIFY_WD_RE_3S,         // Re-detect 3s (2 beeps + flash)
    NOTIFY_WD_RE_30S,        // Re-detect 30s (3 beeps + flash)
    NOTIFY_WD_SESSION_START, // Session started (ascending beeps)
    // Boot
    NOTIFY_BOOT_READY,     // Boot complete (zelda jingle)
    NOTIFY_BOOT_HOLD,      // Boot button held (ascending)
    NOTIFY_SCAN_MODE_BOOT, // Scan mode activated (distinct pattern)
};

void notify(NotifyEvent event);

} // namespace hal
