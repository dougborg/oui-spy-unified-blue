/*
 * OUI SPY - Unified Firmware Orchestrator
 * colonelpanichacks
 *
 * All detection modules run simultaneously with a single web dashboard.
 * No reboot needed to switch features — everything runs at once.
 * Connect to WiFi AP "oui-spy" and open http://192.168.4.1
 */

#include <Arduino.h>
#include <Preferences.h>
#include <nvs_flash.h>

// HAL
#include "hal/pins.h"
#include "hal/buzzer.h"
#include "hal/led.h"
#include "hal/neopixel.h"
#include "hal/mac_util.h"
#include "hal/gps.h"
#include "hal/ble_mgr.h"
#include "hal/wifi_mgr.h"

// Modules
#include "modules/module.h"
#include "modules/detector.h"
#include "modules/foxhunter.h"
#include "modules/flockyou.h"
#include "modules/skyspy.h"

// Web
#include "web/server.h"

// ============================================================================
// Module Instances
// ============================================================================

static DetectorModule   modDetector;
static FoxhunterModule  modFoxhunter;
static FlockyouModule   modFlockyou;
static SkySpyModule     modSkyspy;

static IDetectionModule* modules[] = {
    &modDetector,
    &modFoxhunter,
    &modFlockyou,
    &modSkyspy,
};
static const int MODULE_COUNT = sizeof(modules) / sizeof(modules[0]);

// ============================================================================
// Boot Button (GPIO0) — hold to restart
// ============================================================================

#define BOOT_HOLD_TIME 1500

static unsigned long bootBtnStart = 0;
static bool bootBtnActive = false;

static void checkBootButton() {
    if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
        if (!bootBtnActive) {
            bootBtnActive = true;
            bootBtnStart = millis();
        } else if (millis() - bootBtnStart >= BOOT_HOLD_TIME) {
            Serial.println("\n[OUI-SPY] Boot button held — restarting...");
            hal::buzzerPlay(hal::SND_ASCENDING);
            delay(500);
            ESP.restart();
        }
    } else {
        bootBtnActive = false;
    }
}

// ============================================================================
// Load/Save Module Enabled States (NVS)
// ============================================================================

static void loadModuleStates() {
    Preferences p;
    p.begin("ouispy-mod", true);
    for (int i = 0; i < MODULE_COUNT; i++) {
        bool enabled = p.getBool(modules[i]->name(), true); // default: all enabled
        modules[i]->setEnabled(enabled);
        Serial.printf("[OUI-SPY] Module '%s': %s\n", modules[i]->name(),
                      enabled ? "ENABLED" : "DISABLED");
    }
    p.end();
}

// ============================================================================
// AP Config
// ============================================================================

static String loadAPSSID() {
    Preferences p;
    p.begin("ouispy-ap", true);
    String ssid = p.getString("ssid", "oui-spy");
    String pass = p.getString("pass", "ouispy123");
    p.end();
    return ssid;
}

static String loadAPPass() {
    Preferences p;
    p.begin("ouispy-ap", true);
    String pass = p.getString("pass", "ouispy123");
    p.end();
    return pass;
}

// ============================================================================
// Arduino Entry Points
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println("\n\n========================================");
    Serial.println("  OUI SPY UNIFIED FIRMWARE v3.0");
    Serial.println("  All modules running simultaneously");
    Serial.println("========================================\n");

    // Boot button check
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

    // ---- HAL Init ----
    hal::ledInit();
    hal::buzzerInit();
    hal::neopixelInit();

    // Test NeoPixel: pink -> purple flash
    hal::neopixelSetColor(255, 0, 255);
    delay(300);
    hal::neopixelSetColor(128, 0, 255);
    delay(300);

    hal::gpsInit();

    // ---- WiFi Init ----
    String ssid = loadAPSSID();
    String pass = loadAPPass();
    hal::wifiInit(ssid, pass);

    // ---- BLE Init ----
    hal::bleInit();

    // ---- Load Module States ----
    loadModuleStates();

    // ---- Module Setup ----
    for (int i = 0; i < MODULE_COUNT; i++) {
        if (modules[i]->isEnabled()) {
            Serial.printf("[OUI-SPY] Setting up module: %s\n", modules[i]->name());
            modules[i]->setup();
        }
    }

    // ---- Register BLE Listeners ----
    // All four modules implement BLEListener; register directly
    // (dynamic_cast not available with -fno-rtti from NimBLE)
    hal::bleAddListener(&modDetector);
    hal::bleAddListener(&modFoxhunter);
    hal::bleAddListener(&modFlockyou);
    hal::bleAddListener(&modSkyspy);

    // ---- Web Server Setup ----
    web::serverInit();
    web::registerSystemRoutes(modules, MODULE_COUNT);

    // Register each module's web routes
    AsyncWebServer& server = web::getServer();
    for (int i = 0; i < MODULE_COUNT; i++) {
        modules[i]->registerRoutes(server);
    }

    web::serverBegin();

    // ---- Start BLE Scanning ----
    hal::bleUpdate(); // Kick off first scan

    // ---- Boot Jingle ----
    hal::buzzerPlay(hal::SND_ZELDA_SECRET);

    Serial.println("\n========================================");
    Serial.printf("  WiFi AP: %s\n", ssid.c_str());
    Serial.printf("  Dashboard: http://%s\n", hal::wifiGetAPIP().toString().c_str());
    Serial.println("  All modules active!");
    Serial.println("========================================\n");
}

void loop() {
    // Boot button check (hold 1.5s to restart)
    checkBootButton();

    // GPS update (reads UART data)
    hal::gpsUpdate();

    // BLE scan management (restart scan as needed)
    hal::bleUpdate();

    // Module loops
    for (int i = 0; i < MODULE_COUNT; i++) {
        if (modules[i]->isEnabled()) {
            modules[i]->loop();
        }
    }

    // HAL updates (non-blocking buzzer + neopixel animations)
    hal::buzzerUpdate();
    hal::neopixelUpdate();

    // WiFi manager update
    hal::wifiUpdate();

    // Small yield to prevent WDT
    delay(10);
}
