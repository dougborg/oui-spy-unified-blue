#include "foxhunter.h"
#include "../hal/ble_mgr.h"
#include "../hal/buzzer.h"
#include "../hal/led.h"
#include "../hal/neopixel.h"
#include <Preferences.h>

// ============================================================================
// Foxhunter Module: Single-target RSSI proximity tracker
// Refactored from raw/foxhunter.cpp
// ============================================================================

static String foxTargetMAC = "";
static bool foxEnabled = true;
static bool foxSessionFirstDetection = true;
static bool foxTargetDetected = false;
static int foxCurrentRSSI = -100;
static unsigned long foxLastTargetSeen = 0;
static unsigned long foxLastRSSIPrint = 0;

// ============================================================================
// NVS Persistence
// ============================================================================

static void foxLoadConfig() {
    Preferences p;
    p.begin("tracker", true);
    foxTargetMAC = p.getString("targetMAC", "");
    p.end();
    if (foxTargetMAC.length() > 0)
        foxTargetMAC.toUpperCase();
    Serial.printf("[FOXHUNTER] Target: %s\n",
                  foxTargetMAC.length() > 0 ? foxTargetMAC.c_str() : "(none)");
}

static void foxSaveConfig() {
    Preferences p;
    p.begin("tracker", false);
    p.putString("targetMAC", foxTargetMAC);
    p.end();
}

// ============================================================================
// Module Implementation
// ============================================================================

void FoxhunterModule::setup() {
    foxLoadConfig();
    if (foxTargetMAC.length() > 0) {
        // Request aggressive scan for proximity tracking
        hal::bleRequestAggressiveScan(true);
    }
}

void FoxhunterModule::loop() {
    if (!_enabled)
        return;

    unsigned long now = millis();

    // Proximity beeping when target is in range
    if (foxTargetDetected && (now - foxLastTargetSeen < 5000)) {
        hal::buzzerSetProximity(true, foxCurrentRSSI);

        // RSSI serial output every 2s
        if (now - foxLastRSSIPrint >= 2000) {
            Serial.printf("{\"module\":\"foxhunter\",\"rssi\":%d}\n", foxCurrentRSSI);
            foxLastRSSIPrint = now;
        }
    } else if (foxTargetDetected && (now - foxLastTargetSeen >= 5000)) {
        // Target lost
        foxTargetDetected = false;
        hal::buzzerSetProximity(false);
        Serial.println("{\"module\":\"foxhunter\",\"status\":\"target_lost\"}");
    }
}

void FoxhunterModule::onBLEAdvertisement(NimBLEAdvertisedDevice* device) {
    if (!_enabled || foxTargetMAC.length() == 0)
        return;

    String mac = device->getAddress().toString().c_str();
    mac.toUpperCase();

    if (mac == foxTargetMAC) {
        foxCurrentRSSI = device->getRSSI();
        foxLastTargetSeen = millis();

        if (!foxTargetDetected) {
            foxTargetDetected = true;
            if (foxSessionFirstDetection) {
                hal::buzzerPlay(hal::SND_FOX_FIRST);
                foxSessionFirstDetection = false;
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
    server.on("/api/foxhunter/status", HTTP_GET, [](AsyncWebServerRequest* r) {
        char buf[200];
        snprintf(buf, sizeof(buf),
                 "{\"target\":\"%s\",\"detected\":%s,\"rssi\":%d,\"lastSeen\":%lu}",
                 foxTargetMAC.c_str(), foxTargetDetected ? "true" : "false", foxCurrentRSSI,
                 foxLastTargetSeen);
        r->send(200, "application/json", buf);
    });

    // Set target MAC
    server.on("/api/foxhunter/target", HTTP_POST, [this](AsyncWebServerRequest* r) {
        if (r->hasParam("mac", true)) {
            foxTargetMAC = r->getParam("mac", true)->value();
            foxTargetMAC.trim();
            foxTargetMAC.toUpperCase();
            foxSaveConfig();
            foxTargetDetected = false;
            foxSessionFirstDetection = true;
            foxCurrentRSSI = -100;
            // Request aggressive scan when we have a target
            if (foxTargetMAC.length() > 0) {
                hal::bleRequestAggressiveScan(true);
            } else {
                hal::bleRequestAggressiveScan(false);
            }
            r->send(200, "application/json", "{\"target\":\"" + foxTargetMAC + "\"}");
        } else {
            r->send(400, "application/json", "{\"error\":\"missing mac\"}");
        }
    });

    // Get live RSSI (for polling)
    server.on("/api/foxhunter/rssi", HTTP_GET, [](AsyncWebServerRequest* r) {
        char buf[64];
        snprintf(buf, sizeof(buf), "{\"rssi\":%d,\"detected\":%s}", foxCurrentRSSI,
                 foxTargetDetected ? "true" : "false");
        r->send(200, "application/json", buf);
    });

    // Clear target
    server.on("/api/foxhunter/clear", HTTP_POST, [this](AsyncWebServerRequest* r) {
        foxTargetMAC = "";
        foxSaveConfig();
        foxTargetDetected = false;
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
    foxEnabled = enabled;
    if (!enabled) {
        hal::buzzerSetProximity(false);
        hal::bleRequestAggressiveScan(false);
    }
}
