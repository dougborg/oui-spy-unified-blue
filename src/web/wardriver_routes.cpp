#include "../hal/gps.h"
#include "../modules/wardriver.h"
#include "../storage/nvs_store.h"
#include "routes.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

void registerWardriverRoutes(AsyncWebServer& server, WardriverModule& mod) {
    // Status
    server.on("/api/wardriver/status", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        JsonDocument doc;
        doc["active"] = mod.sessionActive();
        doc["wifi"] = mod.wifiCount();
        doc["ble"] = mod.bleCount();
        doc["unique"] = mod.uniqueCount();
        doc["csv_size"] = (unsigned long)mod.csvFileSize();
        doc["has_csv"] = mod.hasCsvFile();
        doc["gps_fresh"] = hal::gpsIsFresh();
        String json;
        serializeJson(doc, json);
        r->send(200, "application/json", json);
    });

    // Start session
    server.on("/api/wardriver/start", HTTP_POST, [&mod](AsyncWebServerRequest* r) {
        mod.startSession();
        r->send(200, "application/json", "{\"ok\":true}");
    });

    // Stop session
    server.on("/api/wardriver/stop", HTTP_POST, [&mod](AsyncWebServerRequest* r) {
        mod.stopSession();
        r->send(200, "application/json", "{\"ok\":true}");
    });

    // Clear data
    server.on("/api/wardriver/clear", HTTP_POST, [&mod](AsyncWebServerRequest* r) {
        mod.clearData();
        r->send(200, "application/json", "{\"ok\":true}");
    });

    // Recent sightings
    server.on("/api/wardriver/recent", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        std::vector<WDRecentSighting> recent;
        mod.getRecentSightings(recent);

        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        unsigned long now = millis();
        for (const auto& s : recent) {
            JsonObject obj = arr.add<JsonObject>();
            obj["mac"] = s.mac;
            obj["ssid"] = s.ssid;
            obj["type"] = s.type;
            obj["rssi"] = s.rssi;
            obj["channel"] = s.channel;
            obj["ago"] = (now >= s.timestamp) ? (now - s.timestamp) / 1000 : 0;
        }
        String json;
        serializeJson(doc, json);
        r->send(200, "application/json", json);
    });

    // Download CSV file
    server.on("/api/wardriver/download", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        if (!mod.hasCsvFile()) {
            r->send(404, "application/json", "{\"error\":\"no CSV file\"}");
            return;
        }
        r->send(LittleFS, "/wigle.csv", "text/csv", true);
    });

    // WiGLE config - GET
    server.on("/api/wardriver/config", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        JsonDocument doc;
        doc["api_name"] = mod.wigleApiName();
        doc["api_token"] = mod.wigleApiToken().length() > 0 ? "****" : "";
        String json;
        serializeJson(doc, json);
        r->send(200, "application/json", json);
    });

    // WiGLE config - POST
    server.on("/api/wardriver/config", HTTP_POST, [&mod](AsyncWebServerRequest* r) {
        String apiName = r->hasParam("api_name", true) ? r->getParam("api_name", true)->value() : "";
        String apiToken =
            r->hasParam("api_token", true) ? r->getParam("api_token", true)->value() : "";
        mod.setWigleConfig(apiName, apiToken);
        r->send(200, "application/json", "{\"ok\":true}");
    });

    // Filters - GET
    server.on("/api/wardriver/filters", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        std::vector<WDTargetFilter> filters;
        mod.getFilters(filters);
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (const auto& f : filters) {
            JsonObject obj = arr.add<JsonObject>();
            obj["id"] = f.identifier;
            obj["full"] = f.isFullMAC;
            obj["desc"] = f.description;
        }
        String json;
        serializeJson(doc, json);
        r->send(200, "application/json", json);
    });

    // Filters - POST
    server.on("/api/wardriver/filters", HTTP_POST, [&mod](AsyncWebServerRequest* r) {
        mod.parseFiltersFromRequest(r);
        mod.saveFilters();
        JsonDocument doc;
        doc["saved"] = mod.filterCount();
        String json;
        serializeJson(doc, json);
        r->send(200, "application/json", json);
    });

    // Watchlist devices
    server.on("/api/wardriver/devices", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        std::vector<WDDeviceInfo> devices;
        mod.getWatchlistDevices(devices);
        JsonDocument doc;
        JsonArray arr = doc["devices"].to<JsonArray>();
        unsigned long now = millis();
        for (const auto& d : devices) {
            JsonObject obj = arr.add<JsonObject>();
            obj["mac"] = d.macAddress;
            obj["rssi"] = d.rssi;
            obj["filter"] = d.filterDescription;
            obj["alias"] = mod.getAlias(d.macAddress);
            unsigned long ts = (now >= d.lastSeen) ? (now - d.lastSeen) : 0;
            obj["timeSince"] = ts;
        }
        String json;
        serializeJson(doc, json);
        r->send(200, "application/json", json);
    });

    // Set alias
    server.on("/api/wardriver/alias", HTTP_POST, [&mod](AsyncWebServerRequest* r) {
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

    // Clear watchlist devices
    server.on("/api/wardriver/clear-devices", HTTP_POST, [&mod](AsyncWebServerRequest* r) {
        mod.clearWatchlistDevices();
        r->send(200, "application/json", "{\"success\":true}");
    });

    // Clear filters
    server.on("/api/wardriver/clear-filters", HTTP_POST, [&mod](AsyncWebServerRequest* r) {
        mod.clearFilters();
        r->send(200, "application/json", "{\"success\":true}");
    });

    Serial.println("[WARDRIVER] Web routes registered");
}
