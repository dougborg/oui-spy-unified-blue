#include "buzzer.h"
#include "buzzer_logic.h"
#include "led.h"
#include <Preferences.h>

namespace hal {

// ============================================================================
// State machine for non-blocking sound playback
// ============================================================================

static bool _enabled = true;
static SoundEffect _current = SND_NONE;
static SoundPriority _currentPri = PRI_IDLE;
static int _step = 0;
static unsigned long _stepStart = 0;

// Foxhunter proximity state
static bool _toneStarted = false;

static bool _proxEnabled = false;
static int _proxRSSI = -100;
static bool _proxBeeping = false;
static unsigned long _proxLastBeep = 0;

// LEDC channel for buzzer (Arduino-ESP32 core 2.x API)
#define BUZZER_LEDC_CH 0

// Helper: play a tone for a duration (non-blocking step)
static void toneOn(int freq) {
    ledcWriteTone(BUZZER_LEDC_CH, freq);
    ledcWrite(BUZZER_LEDC_CH, 127);
}

static void toneOff() {
    ledcWrite(BUZZER_LEDC_CH, 0);
}

// ============================================================================
// Init
// ============================================================================

void buzzerInit() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    ledcSetup(BUZZER_LEDC_CH, 1000, 8);
    ledcAttachPin(BUZZER_PIN, BUZZER_LEDC_CH);
    ledcWrite(BUZZER_LEDC_CH, 0);

    Preferences p;
    p.begin("ouispy-bz", true);
    _enabled = p.getBool("on", true);
    p.end();
    Serial.printf("[HAL] Buzzer: %s\n", _enabled ? "ON" : "OFF");
}

bool buzzerIsEnabled() {
    return _enabled;
}

void buzzerSetEnabled(bool enabled) {
    _enabled = enabled;
    Preferences p;
    p.begin("ouispy-bz", false);
    p.putBool("on", enabled);
    p.end();
    if (!enabled) {
        toneOff();
        ledOff();
    }
}

bool buzzerIsPlaying() {
    return _current != SND_NONE || _proxEnabled;
}

void buzzerStop() {
    toneOff();
    ledOff();
    _current = SND_NONE;
    _currentPri = PRI_IDLE;
    _step = 0;
}

// ============================================================================
// Sound effect note sequences
// Each returns true when complete.
// ============================================================================

// Pattern: array of {freq, duration_ms, gap_ms} steps
struct Note {
    int freq;
    int dur;
    int gap;
};

// Zelda Secret Discovered: G5 B5 D6 G6(hold)
static const Note zelda[] = {{784, 150, 20}, {988, 150, 20}, {1175, 150, 20}, {1568, 400, 0}};

// Close Encounters: D5 E5 C5 C4 G4(hold)
static const Note closeEnc[] = {
    {587, 120, 30}, {659, 120, 30}, {523, 120, 30}, {262, 120, 30}, {392, 200, 0}};

// Three beeps at 2kHz
static const Note threeBeeps[] = {{2000, 200, 50}, {2000, 200, 50}, {2000, 200, 0}};

// Two beeps at 2kHz
static const Note twoBeeps[] = {{2000, 200, 50}, {2000, 200, 0}};

// Ascending beeps (ready signal)
static const Note ascending[] = {{1900, 200, 100}, {2200, 200, 0}};

// Foxhunter first detection: 3 same-tone beeps at 1kHz
static const Note foxFirst[] = {{1000, 100, 50}, {1000, 100, 50}, {1000, 100, 0}};

// Drone detection: 3 rapid 1kHz beeps
static const Note droneDetect[] = {{1000, 150, 50}, {1000, 150, 50}, {1000, 150, 0}};

// Drone heartbeat: double pulse
static const Note droneHB[] = {{600, 100, 50}, {600, 100, 0}};

// Generic note sequence player. Returns true when done.
static bool playNotes(const Note* notes, int count) {
    unsigned long elapsed = millis() - _stepStart;
    int noteIdx = _step / 2; // each note has tone phase + gap phase
    bool isTone = (_step % 2) == 0;

    if (noteIdx >= count)
        return true;

    const Note& n = notes[noteIdx];

    if (isTone) {
        if (!_toneStarted) {
            if (_enabled)
                toneOn(n.freq);
            ledOn();
            _stepStart = millis();
            _toneStarted = true;
        }
        if (millis() - _stepStart >= (unsigned long)n.dur) {
            toneOff();
            ledOff();
            _step++;
            _toneStarted = false;
            _stepStart = millis();
            if (n.gap == 0 && noteIdx == count - 1)
                return true;
        }
    } else {
        // Gap phase
        if (elapsed >= (unsigned long)n.gap) {
            _step++;
            _stepStart = millis();
        }
    }
    return false;
}

// Crow caw: sweep from startFreq to endFreq with warble
static bool playCrowCaw(int startFreq, int endFreq, int durationMs, int warbleHz) {
    unsigned long elapsed = millis() - _stepStart;
    if (elapsed >= (unsigned long)durationMs) {
        toneOff();
        return true;
    }
    if (_enabled) {
        int steps = durationMs / 8;
        float fStep = (float)(endFreq - startFreq) / steps;
        int currentStep = elapsed / 8;
        int f = startFreq + (int)(fStep * currentStep);
        if (warbleHz > 0 && (currentStep % 3 == 0)) {
            f += ((currentStep % 6 < 3) ? warbleHz : -warbleHz);
        }
        if (f < 100)
            f = 100;
        toneOn(f);
    }
    return false;
}

// Crow call sequence state
static int crowPhase = 0;

static bool playCrowCall() {
    unsigned long elapsed = millis() - _stepStart;
    switch (crowPhase) {
    case 0: // Caw 1
        if (playCrowCaw(850, 380, 180, 40)) {
            crowPhase = 1;
            _stepStart = millis();
        }
        return false;
    case 1: // gap
        if (elapsed >= 100) {
            crowPhase = 2;
            _stepStart = millis();
        }
        return false;
    case 2: // Caw 2
        if (playCrowCaw(780, 350, 150, 50)) {
            crowPhase = 3;
            _stepStart = millis();
        }
        return false;
    case 3: // gap
        if (elapsed >= 100) {
            crowPhase = 4;
            _stepStart = millis();
        }
        return false;
    case 4: // Caw 3
        if (playCrowCaw(820, 280, 220, 60)) {
            crowPhase = 5;
            _stepStart = millis();
        }
        return false;
    case 5: // gap
        if (elapsed >= 80) {
            crowPhase = 6;
            _stepStart = millis();
        }
        return false;
    case 6: // staccato kk-kk
        if (elapsed < 25) {
            if (_enabled)
                toneOn(600);
        } else if (elapsed < 65) {
            toneOff();
        } else if (elapsed < 90) {
            if (_enabled)
                toneOn(550);
        } else {
            toneOff();
            return true;
        }
        return false;
    }
    return true;
}

// Crow alarm (detection): two rising chirps + one descending caw
static int crowAlarmPhase = 0;

static bool playCrowAlarm() {
    unsigned long elapsed = millis() - _stepStart;
    switch (crowAlarmPhase) {
    case 0:
        if (playCrowCaw(400, 900, 100, 30)) {
            crowAlarmPhase = 1;
            _stepStart = millis();
        }
        return false;
    case 1:
        if (elapsed >= 60) {
            crowAlarmPhase = 2;
            _stepStart = millis();
        }
        return false;
    case 2:
        if (playCrowCaw(450, 950, 100, 30)) {
            crowAlarmPhase = 3;
            _stepStart = millis();
        }
        return false;
    case 3:
        if (elapsed >= 60) {
            crowAlarmPhase = 4;
            _stepStart = millis();
        }
        return false;
    case 4:
        if (playCrowCaw(900, 350, 200, 50))
            return true;
        return false;
    }
    return true;
}

// Crow heartbeat
static bool playCrowHeartbeat() {
    unsigned long elapsed = millis() - _stepStart;
    if (_step == 0) {
        if (playCrowCaw(500, 400, 80, 20)) {
            _step = 1;
            _stepStart = millis();
        }
        return false;
    } else if (_step == 1) {
        if (elapsed >= 120) {
            _step = 2;
            _stepStart = millis();
        }
        return false;
    } else {
        return playCrowCaw(480, 380, 80, 20);
    }
}

// ============================================================================
// Foxhunter proximity RSSI -> beep interval mapping
// ============================================================================

static int calcProxInterval(int rssi) {
    return buzzer_logic::calcProximityIntervalMs(rssi);
}

void buzzerSetProximity(bool enabled, int rssi) {
    _proxEnabled = enabled;
    _proxRSSI = rssi;
    if (!enabled) {
        toneOff();
        ledOff();
        _proxBeeping = false;
        if (_currentPri <= PRI_PROXIMITY) {
            _current = SND_NONE;
            _currentPri = PRI_IDLE;
        }
    }
}

static void updateProximity() {
    if (!_proxEnabled)
        return;

    unsigned long now = millis();

    // Ultra close: solid tone
    if (_proxRSSI >= -25) {
        if (_enabled)
            toneOn(1000);
        ledOn();
        _proxBeeping = true;
        return;
    }

    int interval = calcProxInterval(_proxRSSI);

    if (_proxBeeping) {
        if (now - _proxLastBeep >= 50) { // 50ms beep duration
            toneOff();
            ledOff();
            _proxBeeping = false;
        }
    } else {
        if (now - _proxLastBeep >= (unsigned long)interval) {
            if (_enabled)
                toneOn(1000);
            ledOn();
            _proxBeeping = true;
            _proxLastBeep = now;
        }
    }
}

// ============================================================================
// Play a sound effect (preempts lower priority)
// ============================================================================

void buzzerPlay(SoundEffect effect) {
    SoundPriority pri;
    switch (effect) {
    case SND_ZELDA_SECRET:
    case SND_CROW_CALL:
    case SND_CLOSE_ENCOUNTERS:
    case SND_ASCENDING:
        pri = PRI_BOOT;
        break;
    case SND_CROW_HEARTBEAT:
    case SND_DRONE_HEARTBEAT:
        pri = PRI_HEARTBEAT;
        break;
    case SND_THREE_BEEPS:
    case SND_TWO_BEEPS:
    case SND_CROW_ALARM:
    case SND_DRONE_DETECT:
    case SND_FOX_FIRST:
        pri = PRI_DETECTION;
        break;
    default:
        pri = PRI_IDLE;
        break;
    }

    if (pri < _currentPri && _current != SND_NONE)
        return; // lower priority, skip

    toneOff();
    ledOff();
    _current = effect;
    _currentPri = pri;
    _step = 0;
    _toneStarted = false;
    _stepStart = millis();
    crowPhase = 0;
    crowAlarmPhase = 0;
}

// ============================================================================
// Main update (call every loop iteration)
// ============================================================================

void buzzerUpdate() {
    // Proximity mode takes precedence if active and no higher-priority sound
    if (_proxEnabled && _current == SND_NONE) {
        updateProximity();
        return;
    }

    if (_current == SND_NONE)
        return;

    bool done = false;
    switch (_current) {
    case SND_ZELDA_SECRET:
        done = playNotes(zelda, 4);
        break;
    case SND_CLOSE_ENCOUNTERS:
        done = playNotes(closeEnc, 5);
        break;
    case SND_THREE_BEEPS:
        done = playNotes(threeBeeps, 3);
        break;
    case SND_TWO_BEEPS:
        done = playNotes(twoBeeps, 2);
        break;
    case SND_ASCENDING:
        done = playNotes(ascending, 2);
        break;
    case SND_FOX_FIRST:
        done = playNotes(foxFirst, 3);
        break;
    case SND_DRONE_DETECT:
        done = playNotes(droneDetect, 3);
        break;
    case SND_DRONE_HEARTBEAT:
        done = playNotes(droneHB, 2);
        break;
    case SND_CROW_CALL:
        done = playCrowCall();
        break;
    case SND_CROW_ALARM:
        done = playCrowAlarm();
        break;
    case SND_CROW_HEARTBEAT:
        done = playCrowHeartbeat();
        break;
    default:
        done = true;
        break;
    }

    if (done) {
        toneOff();
        ledOff();
        _current = SND_NONE;
        _currentPri = PRI_IDLE;
        _step = 0;
    }
}

} // namespace hal
