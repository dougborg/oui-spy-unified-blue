#include "notify.h"
#include "buzzer.h"
#include "neopixel.h"

namespace hal {

void notify(NotifyEvent event) {
    switch (event) {
    // Detector events
    case NOTIFY_DET_NEW_DEVICE:
        buzzerPlay(SND_THREE_BEEPS);
        neopixelFlash(240, 300, 270);
        break;
    case NOTIFY_DET_RE_3S:
        buzzerPlay(SND_TWO_BEEPS);
        neopixelFlash(240, 300, 270);
        break;
    case NOTIFY_DET_RE_30S:
        buzzerPlay(SND_THREE_BEEPS);
        neopixelFlash(240, 300, 270);
        break;

    // Flockyou events
    case NOTIFY_FY_ALERT:
        buzzerPlay(SND_CROW_ALARM);
        neopixelFlash(0, 300, 0);
        break;
    case NOTIFY_FY_HEARTBEAT:
        buzzerPlay(SND_CROW_HEARTBEAT);
        neopixelSetState(NEO_HEARTBEAT_GLOW, 300);
        break;
    case NOTIFY_FY_IDLE:
        neopixelSetState(NEO_IDLE_BREATHING);
        break;

    // Skyspy events
    case NOTIFY_SS_DRONE:
        buzzerPlay(SND_DRONE_DETECT);
        break;
    case NOTIFY_SS_HEARTBEAT:
        buzzerPlay(SND_DRONE_HEARTBEAT);
        break;

    // Foxhunter events
    case NOTIFY_FOX_ACQUIRED:
        buzzerPlay(SND_FOX_FIRST);
        break;

    // Boot events
    case NOTIFY_BOOT_READY:
        buzzerPlay(SND_ZELDA_SECRET);
        break;
    case NOTIFY_BOOT_HOLD:
        buzzerPlay(SND_ASCENDING);
        break;
    }
}

} // namespace hal
