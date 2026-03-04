#include "../modules/detector.h"
#include "../storage/nvs_store.h"
#include "routes.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

void registerDetectorRoutes(AsyncWebServer& server, DetectorModule& mod) {
    // Get filters
    server.on("/api/detector/filters", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        const auto& filters = mod.filters();
        for (int i = 0; i < (int)filters.size(); i++) {
            JsonObject obj = arr.add<JsonObject>();
            obj["id"] = filters[i].identifier;
            obj["full"] = filters[i].isFullMAC;
            obj["desc"] = filters[i].description;
        }
        String json;
        serializeJson(doc, json);
        r->send(200, "application/json", json);
    });

    // Save filters
    server.on("/api/detector/filters", HTTP_POST, [&mod](AsyncWebServerRequest* r) {
        mod.parseFiltersFromRequest(r);
        mod.setBuzzerEnabled(r->hasParam("buzzerEnabled", true));
        mod.setLedEnabled(r->hasParam("ledEnabled", true));
        mod.saveFilters();
        JsonDocument doc;
        doc["saved"] = (int)mod.filters().size();
        String json;
        serializeJson(doc, json);
        r->send(200, "application/json", json);
    });

    // Get detected devices
    server.on("/api/detector/devices", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        JsonDocument doc;
        JsonArray arr = doc["devices"].to<JsonArray>();
        unsigned long now = millis();
        const auto& devices = mod.devices();
        for (int i = 0; i < (int)devices.size(); i++) {
            JsonObject obj = arr.add<JsonObject>();
            obj["mac"] = devices[i].macAddress;
            obj["rssi"] = devices[i].rssi;
            obj["filter"] = devices[i].filterDescription;
            obj["alias"] = mod.getAlias(devices[i].macAddress);
            unsigned long ts = (now >= devices[i].lastSeen) ? (now - devices[i].lastSeen) : 0;
            obj["timeSince"] = ts;
        }
        String json;
        serializeJson(doc, json);
        r->send(200, "application/json", json);
    });

    // Save alias (with length validation)
    server.on("/api/detector/alias", HTTP_POST, [&mod](AsyncWebServerRequest* r) {
        if (r->hasParam("mac", true) && r->hasParam("alias", true)) {
            String alias = r->getParam("alias", true)->value();
            if (alias.length() > 32) {
                r->send(400, "application/json", "{\"error\":\"alias max 32 characters\"}");
                return;
            }
            mod.setAlias(r->getParam("mac", true)->value(), alias);
            mod.saveAliases();
            r->send(200, "application/json", "{\"success\":true}");
        } else {
            r->send(400, "application/json", "{\"error\":\"missing params\"}");
        }
    });

    // Clear device history
    server.on("/api/detector/clear-devices", HTTP_POST, [&mod](AsyncWebServerRequest* r) {
        mod.clearDevices();
        r->send(200, "application/json", "{\"success\":true}");
    });

    // Clear filters
    server.on("/api/detector/clear-filters", HTTP_POST, [&mod](AsyncWebServerRequest* r) {
        mod.clearFilters();
        r->send(200, "application/json", "{\"success\":true}");
    });

    Serial.println("[DETECTOR] Web routes registered");
}
