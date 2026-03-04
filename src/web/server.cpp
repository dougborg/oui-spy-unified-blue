#include "server.h"
#include "../hal/buzzer.h"
#include "../hal/gps.h"
#include "../storage/nvs_store.h"
#include "dashboard.h"
#include <ArduinoJson.h>

namespace web {

static AsyncWebServer _server(80);
static IModule** _modules = nullptr;
static int _moduleCount = 0;

void serverInit() {
    // Nothing special needed
}

AsyncWebServer& getServer() {
    return _server;
}

void registerSystemRoutes(IModule** modules, int count) {
    _modules = modules;
    _moduleCount = count;

    // Dashboard HTML
    _server.on("/", HTTP_GET,
               [](AsyncWebServerRequest* r) { r->send(200, "text/html", DASHBOARD_HTML); });

    // System status
    _server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* r) {
        JsonDocument doc;
        doc["uptime"] = millis() / 1000;
        doc["heap"] = ESP.getFreeHeap();
        doc["psram"] = ESP.getFreePsram();
        doc["buzzer"] = hal::buzzerIsEnabled();
        String json;
        serializeJson(doc, json);
        r->send(200, "application/json", json);
    });

    // Module list with enabled state
    _server.on("/api/modules", HTTP_GET, [](AsyncWebServerRequest* r) {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (int i = 0; i < _moduleCount; i++) {
            JsonObject obj = arr.add<JsonObject>();
            obj["name"] = _modules[i]->name();
            obj["enabled"] = _modules[i]->isEnabled();
        }
        String json;
        serializeJson(doc, json);
        r->send(200, "application/json", json);
    });

    // Toggle module
    _server.on("/api/modules/toggle", HTTP_POST, [](AsyncWebServerRequest* r) {
        if (!r->hasParam("name", true) || !r->hasParam("enabled", true)) {
            r->send(400, "application/json", "{\"error\":\"missing params\"}");
            return;
        }
        String name = r->getParam("name", true)->value();
        bool enabled = r->getParam("enabled", true)->value() == "true";
        for (int i = 0; i < _moduleCount; i++) {
            if (name == _modules[i]->name()) {
                _modules[i]->setEnabled(enabled);
                storage::setModuleEnabled(name.c_str(), enabled);
                r->send(200, "application/json", "{\"ok\":true}");
                return;
            }
        }
        r->send(404, "application/json", "{\"error\":\"module not found\"}");
    });

    // Buzzer toggle
    _server.on("/api/buzzer", HTTP_POST, [](AsyncWebServerRequest* r) {
        if (r->hasParam("on", true)) {
            bool on = r->getParam("on", true)->value() == "true";
            hal::buzzerSetEnabled(on);
            JsonDocument doc;
            doc["buzzer"] = on;
            String json;
            serializeJson(doc, json);
            r->send(200, "application/json", json);
        } else {
            r->send(400, "application/json", "{\"error\":\"missing on param\"}");
        }
    });

    // GPS status
    _server.on("/api/gps", HTTP_GET, [](AsyncWebServerRequest* r) {
        const hal::GPSData& g = hal::gpsGet();
        JsonDocument doc;
        doc["valid"] = g.valid;
        doc["lat"] = serialized(String(g.lat, 8));
        doc["lon"] = serialized(String(g.lon, 8));
        doc["acc"] = serialized(String(g.accuracy, 1));
        doc["hardware"] = g.isHardware;
        doc["hw_detected"] = g.hwDetected;
        doc["hw_fix"] = g.hwFix;
        doc["sats"] = g.satellites;
        doc["fresh"] = hal::gpsIsFresh();
        String json;
        serializeJson(doc, json);
        r->send(200, "application/json", json);
    });

    // AP settings
    _server.on("/api/ap", HTTP_GET, [](AsyncWebServerRequest* r) {
        JsonDocument doc;
        doc["ssid"] = storage::getAPSSID();
        String json;
        serializeJson(doc, json);
        r->send(200, "application/json", json);
    });

    _server.on("/api/ap", HTTP_POST, [](AsyncWebServerRequest* r) {
        if (r->hasParam("ssid", true)) {
            String ssid = r->getParam("ssid", true)->value();
            String pass = r->hasParam("pass", true) ? r->getParam("pass", true)->value() : "";
            if (ssid.length() < 1 || ssid.length() > 32) {
                r->send(400, "application/json", "{\"error\":\"SSID must be 1-32 characters\"}");
                return;
            }
            if (pass.length() > 0 && (pass.length() < 8 || pass.length() > 63)) {
                r->send(400, "application/json",
                        "{\"error\":\"Password must be 8-63 characters or empty for open\"}");
                return;
            }
            storage::setAPConfig(ssid, pass);
            r->send(200, "application/json", "{\"ok\":true,\"reboot\":true}");
            delay(500);
            ESP.restart();
        } else {
            r->send(400, "application/json", "{\"error\":\"missing ssid\"}");
        }
    });

    // Device reset
    _server.on("/api/reset", HTTP_POST, [](AsyncWebServerRequest* r) {
        r->send(200, "application/json", "{\"ok\":true}");
        delay(500);
        ESP.restart();
    });
}

void serverBegin() {
    _server.begin();
    Serial.println("[WEB] Server started on port 80");
}

} // namespace web
