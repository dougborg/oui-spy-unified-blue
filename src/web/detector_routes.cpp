#include "../modules/detector.h"
#include "../storage/nvs_store.h"
#include "routes.h"
#include <ESPAsyncWebServer.h>

void registerDetectorRoutes(AsyncWebServer& server, DetectorModule& mod) {
    // Get filters
    server.on("/api/detector/filters", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        String json = "[";
        const auto& filters = mod.filters();
        for (int i = 0; i < (int)filters.size(); i++) {
            if (i > 0)
                json += ",";
            json += "{\"id\":\"" + filters[i].identifier +
                    "\",\"full\":" + (filters[i].isFullMAC ? "true" : "false") + ",\"desc\":\"" +
                    filters[i].description + "\"}";
        }
        json += "]";
        r->send(200, "application/json", json);
    });

    // Save filters
    server.on("/api/detector/filters", HTTP_POST, [&mod](AsyncWebServerRequest* r) {
        mod.parseFiltersFromRequest(r);
        mod.setBuzzerEnabled(r->hasParam("buzzerEnabled", true));
        mod.setLedEnabled(r->hasParam("ledEnabled", true));
        mod.saveFilters();
        r->send(200, "application/json", "{\"saved\":" + String(mod.filters().size()) + "}");
    });

    // Get detected devices
    server.on("/api/detector/devices", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        String json = "{\"devices\":[";
        unsigned long now = millis();
        const auto& devices = mod.devices();
        for (int i = 0; i < (int)devices.size(); i++) {
            if (i > 0)
                json += ",";
            String alias = mod.getAlias(devices[i].macAddress);
            unsigned long ts = (now >= devices[i].lastSeen) ? (now - devices[i].lastSeen) : 0;
            json += "{\"mac\":\"" + devices[i].macAddress +
                    "\",\"rssi\":" + String(devices[i].rssi) + ",\"filter\":\"" +
                    devices[i].filterDescription + "\",\"alias\":\"" + alias +
                    "\",\"timeSince\":" + String(ts) + "}";
        }
        json += "]}";
        r->send(200, "application/json", json);
    });

    // Save alias
    server.on("/api/detector/alias", HTTP_POST, [&mod](AsyncWebServerRequest* r) {
        if (r->hasParam("mac", true) && r->hasParam("alias", true)) {
            mod.setAlias(r->getParam("mac", true)->value(), r->getParam("alias", true)->value());
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
