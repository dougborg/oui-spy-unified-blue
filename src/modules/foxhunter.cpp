#include "foxhunter.h"
#include "../hal/ble_mgr.h"
#include "../hal/buzzer.h"
#include "../hal/led.h"
#include "../hal/neopixel.h"
#include <Preferences.h>

// ============================================================================
// Foxhunter Module: Single-target RSSI proximity tracker
// ============================================================================

// ============================================================================
// NVS Persistence
// ============================================================================

void FoxhunterModule::loadConfig() {
    Preferences p;
    p.begin("tracker", true);
    _targetMAC = p.getString("targetMAC", "");
    p.end();
    if (_targetMAC.length() > 0)
        _targetMAC.toUpperCase();
    Serial.printf("[FOXHUNTER] Target: %s\n",
                  _targetMAC.length() > 0 ? _targetMAC.c_str() : "(none)");
}

void FoxhunterModule::saveConfig() {
    Preferences p;
    p.begin("tracker", false);
    p.putString("targetMAC", _targetMAC);
    p.end();
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
// Web Routes: /api/foxhunter/*
// ============================================================================

void FoxhunterModule::registerRoutes(AsyncWebServer& server) {
    // Get current target and RSSI
    server.on("/api/foxhunter/status", HTTP_GET, [this](AsyncWebServerRequest* r) {
        char buf[200];
        snprintf(
            buf, sizeof(buf), "{\"target\":\"%s\",\"detected\":%s,\"rssi\":%d,\"lastSeen\":%lu}",
            _targetMAC.c_str(), _targetDetected ? "true" : "false", _currentRSSI, _lastTargetSeen);
        r->send(200, "application/json", buf);
    });

    // Set target MAC
    server.on("/api/foxhunter/target", HTTP_POST, [this](AsyncWebServerRequest* r) {
        if (r->hasParam("mac", true)) {
            _targetMAC = r->getParam("mac", true)->value();
            _targetMAC.trim();
            _targetMAC.toUpperCase();
            saveConfig();
            _targetDetected = false;
            _sessionFirstDetection = true;
            _currentRSSI = -100;
            // Request aggressive scan when we have a target
            if (_targetMAC.length() > 0) {
                hal::bleRequestAggressiveScan(true);
            } else {
                hal::bleRequestAggressiveScan(false);
            }
            r->send(200, "application/json", "{\"target\":\"" + _targetMAC + "\"}");
        } else {
            r->send(400, "application/json", "{\"error\":\"missing mac\"}");
        }
    });

    // Get live RSSI (for polling)
    server.on("/api/foxhunter/rssi", HTTP_GET, [this](AsyncWebServerRequest* r) {
        char buf[64];
        snprintf(buf, sizeof(buf), "{\"rssi\":%d,\"detected\":%s}", _currentRSSI,
                 _targetDetected ? "true" : "false");
        r->send(200, "application/json", buf);
    });

    // Clear target
    server.on("/api/foxhunter/clear", HTTP_POST, [this](AsyncWebServerRequest* r) {
        _targetMAC = "";
        saveConfig();
        _targetDetected = false;
        hal::buzzerSetProximity(false);
        hal::bleRequestAggressiveScan(false);
        r->send(200, "application/json", "{\"cleared\":true}");
    });

    Serial.println("[FOXHUNTER] Web routes registered");
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
