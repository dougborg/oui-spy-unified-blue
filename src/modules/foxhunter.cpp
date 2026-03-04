#include "foxhunter.h"
#include "../hal/ble_mgr.h"
#include "../hal/buzzer.h"
#include "../hal/led.h"
#include "../hal/neopixel.h"
#include "../storage/nvs_store.h"
#include "../web/routes.h"

// ============================================================================
// Foxhunter Module: Single-target RSSI proximity tracker
// ============================================================================

// ============================================================================
// NVS Persistence
// ============================================================================

void FoxhunterModule::loadConfig() {
    _targetMAC = storage::getFoxTargetMAC();
    if (_targetMAC.length() > 0)
        _targetMAC.toUpperCase();
    Serial.printf("[FOXHUNTER] Target: %s\n",
                  _targetMAC.length() > 0 ? _targetMAC.c_str() : "(none)");
}

void FoxhunterModule::saveConfig() {
    storage::setFoxTargetMAC(_targetMAC);
}

// ============================================================================
// Module Implementation
// ============================================================================

void FoxhunterModule::setup() {
    loadConfig();
    if (_targetMAC.length() > 0) {
        // Request aggressive scan for proximity tracking
        hal::bleRequestAggressiveScan(true);
    }
}

void FoxhunterModule::loop() {
    if (!_enabled)
        return;

    unsigned long now = millis();

    // Proximity beeping when target is in range
    if (_targetDetected && (now - _lastTargetSeen < 5000)) {
        hal::buzzerSetProximity(true, _currentRSSI);

        // RSSI serial output every 2s
        if (now - _lastRSSIPrint >= 2000) {
            Serial.printf("{\"module\":\"foxhunter\",\"rssi\":%d}\n", _currentRSSI);
            _lastRSSIPrint = now;
        }
    } else if (_targetDetected && (now - _lastTargetSeen >= 5000)) {
        // Target lost
        _targetDetected = false;
        hal::buzzerSetProximity(false);
        Serial.println("{\"module\":\"foxhunter\",\"status\":\"target_lost\"}");
    }
}

void FoxhunterModule::onBLEAdvertisement(NimBLEAdvertisedDevice* device) {
    if (!_enabled || _targetMAC.length() == 0)
        return;

    String mac = device->getAddress().toString().c_str();
    mac.toUpperCase();

    if (mac == _targetMAC) {
        _currentRSSI = device->getRSSI();
        _lastTargetSeen = millis();

        if (!_targetDetected) {
            _targetDetected = true;
            if (_sessionFirstDetection) {
                hal::buzzerPlay(hal::SND_FOX_FIRST);
                _sessionFirstDetection = false;
                Serial.println("{\"module\":\"foxhunter\",\"status\":\"target_acquired\"}");
            }
        }
    }
}

// ============================================================================
// Public Operations (used by route handlers)
// ============================================================================

void FoxhunterModule::setTarget(const String& mac) {
    _targetMAC = mac;
    _targetMAC.trim();
    _targetMAC.toUpperCase();
    saveConfig();
    _targetDetected = false;
    _sessionFirstDetection = true;
    _currentRSSI = -100;
    hal::bleRequestAggressiveScan(_targetMAC.length() > 0);
}

void FoxhunterModule::clearTarget() {
    _targetMAC = "";
    saveConfig();
    _targetDetected = false;
    hal::buzzerSetProximity(false);
    hal::bleRequestAggressiveScan(false);
}

// ============================================================================
// Web Routes
// ============================================================================

void FoxhunterModule::registerRoutes(AsyncWebServer& server) {
    registerFoxhunterRoutes(server, *this);
}

bool FoxhunterModule::isEnabled() {
    return _enabled;
}
void FoxhunterModule::setEnabled(bool enabled) {
    _enabled = enabled;
    if (!enabled) {
        hal::buzzerSetProximity(false);
        hal::bleRequestAggressiveScan(false);
    }
}
