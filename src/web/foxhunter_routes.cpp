#include "../modules/detector_logic.h"
#include "../modules/foxhunter.h"
#include "routes.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

void registerFoxhunterRoutes(AsyncWebServer& server, FoxhunterModule& mod) {
    // Get current target and RSSI
    server.on("/api/foxhunter/status", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        JsonDocument doc;
        doc["target"] = mod.targetMAC();
        doc["detected"] = mod.targetDetected();
        doc["rssi"] = mod.currentRSSI();
        doc["lastSeen"] = mod.lastTargetSeen();
        String json;
        serializeJson(doc, json);
        r->send(200, "application/json", json);
    });

    // Set target MAC (with validation)
    server.on("/api/foxhunter/target", HTTP_POST, [&mod](AsyncWebServerRequest* r) {
        if (r->hasParam("mac", true)) {
            String mac = r->getParam("mac", true)->value();
            mac.trim();
            if (mac.length() > 0 && !detector_logic::isValidMac(mac.c_str())) {
                r->send(400, "application/json", "{\"error\":\"invalid MAC format\"}");
                return;
            }
            mod.setTarget(mac);
            JsonDocument doc;
            doc["target"] = mod.targetMAC();
            String json;
            serializeJson(doc, json);
            r->send(200, "application/json", json);
        } else {
            r->send(400, "application/json", "{\"error\":\"missing mac\"}");
        }
    });

    // Get live RSSI (for polling)
    server.on("/api/foxhunter/rssi", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        JsonDocument doc;
        doc["rssi"] = mod.currentRSSI();
        doc["detected"] = mod.targetDetected();
        String json;
        serializeJson(doc, json);
        r->send(200, "application/json", json);
    });

    // Clear target
    server.on("/api/foxhunter/clear", HTTP_POST, [&mod](AsyncWebServerRequest* r) {
        mod.clearTarget();
        r->send(200, "application/json", "{\"cleared\":true}");
    });

    Serial.println("[FOXHUNTER] Web routes registered");
}
