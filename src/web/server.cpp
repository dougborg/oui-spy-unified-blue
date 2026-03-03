#include "server.h"
#include "dashboard.h"
#include "../hal/buzzer.h"
#include "../hal/gps.h"
#include <Preferences.h>

namespace web {

static AsyncWebServer _server(80);
static IDetectionModule** _modules = nullptr;
static int _moduleCount = 0;

void serverInit() {
    // Nothing special needed
}

AsyncWebServer& getServer() { return _server; }

void registerSystemRoutes(IDetectionModule** modules, int count) {
    _modules = modules;
    _moduleCount = count;

    // Dashboard HTML
    _server.on("/", HTTP_GET, [](AsyncWebServerRequest* r) {
        r->send(200, "text/html", DASHBOARD_HTML);
    });

    // System status
    _server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* r) {
        char buf[256];
        snprintf(buf, sizeof(buf),
            "{\"uptime\":%lu,\"heap\":%u,\"psram\":%u,\"buzzer\":%s}",
            millis() / 1000,
            ESP.getFreeHeap(),
            ESP.getFreePsram(),
            hal::buzzerIsEnabled() ? "true" : "false");
        r->send(200, "application/json", buf);
    });

    // Module list with enabled state
    _server.on("/api/modules", HTTP_GET, [](AsyncWebServerRequest* r) {
        String json = "[";
        for (int i = 0; i < _moduleCount; i++) {
            if (i > 0) json += ",";
            json += "{\"name\":\"";
            json += _modules[i]->name();
            json += "\",\"enabled\":";
            json += _modules[i]->isEnabled() ? "true" : "false";
            json += "}";
        }
        json += "]";
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
                // Save to NVS
                Preferences p;
                p.begin("ouispy-mod", false);
                p.putBool(name.c_str(), enabled);
                p.end();
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
            r->send(200, "application/json", "{\"buzzer\":" + String(on ? "true" : "false") + "}");
        } else {
            r->send(400, "application/json", "{\"error\":\"missing on param\"}");
        }
    });

    // GPS status
    _server.on("/api/gps", HTTP_GET, [](AsyncWebServerRequest* r) {
        const hal::GPSData& g = hal::gpsGet();
        char buf[256];
        snprintf(buf, sizeof(buf),
            "{\"valid\":%s,\"lat\":%.8f,\"lon\":%.8f,\"acc\":%.1f,"
            "\"hardware\":%s,\"hw_detected\":%s,\"hw_fix\":%s,\"sats\":%d,"
            "\"fresh\":%s}",
            g.valid ? "true" : "false", g.lat, g.lon, g.accuracy,
            g.isHardware ? "true" : "false",
            g.hwDetected ? "true" : "false",
            g.hwFix ? "true" : "false", g.satellites,
            hal::gpsIsFresh() ? "true" : "false");
        r->send(200, "application/json", buf);
    });

    // AP settings
    _server.on("/api/ap", HTTP_GET, [](AsyncWebServerRequest* r) {
        Preferences p;
        p.begin("ouispy-ap", true);
        String ssid = p.getString("ssid", "oui-spy");
        p.end();
        r->send(200, "application/json", "{\"ssid\":\"" + ssid + "\"}");
    });

    _server.on("/api/ap", HTTP_POST, [](AsyncWebServerRequest* r) {
        if (r->hasParam("ssid", true)) {
            String ssid = r->getParam("ssid", true)->value();
            String pass = r->hasParam("pass", true) ? r->getParam("pass", true)->value() : "";
            if (ssid.length() >= 1 && ssid.length() <= 32) {
                Preferences p;
                p.begin("ouispy-ap", false);
                p.putString("ssid", ssid);
                p.putString("pass", pass);
                p.end();
                r->send(200, "application/json", "{\"ok\":true,\"reboot\":true}");
                delay(500);
                ESP.restart();
            } else {
                r->send(400, "application/json", "{\"error\":\"invalid ssid\"}");
            }
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
