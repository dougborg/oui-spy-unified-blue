#include "../modules/foxhunter.h"
#include "routes.h"
#include <ESPAsyncWebServer.h>

void registerFoxhunterRoutes(AsyncWebServer& server, FoxhunterModule& mod) {
    // Get current target and RSSI
    server.on("/api/foxhunter/status", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        char buf[200];
        snprintf(buf, sizeof(buf),
                 "{\"target\":\"%s\",\"detected\":%s,\"rssi\":%d,\"lastSeen\":%lu}",
                 mod.targetMAC().c_str(), mod.targetDetected() ? "true" : "false",
                 mod.currentRSSI(), mod.lastTargetSeen());
        r->send(200, "application/json", buf);
    });

    // Set target MAC
    server.on("/api/foxhunter/target", HTTP_POST, [&mod](AsyncWebServerRequest* r) {
        if (r->hasParam("mac", true)) {
            String mac = r->getParam("mac", true)->value();
            mod.setTarget(mac);
            r->send(200, "application/json", "{\"target\":\"" + mod.targetMAC() + "\"}");
        } else {
            r->send(400, "application/json", "{\"error\":\"missing mac\"}");
        }
    });

    // Get live RSSI (for polling)
    server.on("/api/foxhunter/rssi", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        char buf[64];
        snprintf(buf, sizeof(buf), "{\"rssi\":%d,\"detected\":%s}", mod.currentRSSI(),
                 mod.targetDetected() ? "true" : "false");
        r->send(200, "application/json", buf);
    });

    // Clear target
    server.on("/api/foxhunter/clear", HTTP_POST, [&mod](AsyncWebServerRequest* r) {
        mod.clearTarget();
        r->send(200, "application/json", "{\"cleared\":true}");
    });

    Serial.println("[FOXHUNTER] Web routes registered");
}
