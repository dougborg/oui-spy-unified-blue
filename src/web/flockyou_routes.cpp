#include "../hal/gps.h"
#include "../modules/flockyou.h"
#include "routes.h"
#include <ESPAsyncWebServer.h>

void registerFlockyouRoutes(AsyncWebServer& server, FlockyouModule& mod) {
    server.on("/api/flockyou/detections", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        AsyncResponseStream* resp = r->beginResponseStream("application/json");
        mod.writeJSON(resp);
        r->send(resp);
    });

    server.on("/api/flockyou/stats", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        int raven = 0, withGPS = 0;
        SemaphoreHandle_t mtx = mod.mutex();
        if (mtx && xSemaphoreTake(mtx, pdMS_TO_TICKS(100)) == pdTRUE) {
            const FYDetection* dets = mod.detections();
            int count = mod.detectionCount();
            for (int i = 0; i < count; i++) {
                if (dets[i].isRaven)
                    raven++;
                if (dets[i].hasGPS)
                    withGPS++;
            }
            xSemaphoreGive(mtx);
        }
        const hal::GPSData& g = hal::gpsGet();
        const char* gpsSrc = "none";
        if (g.isHardware && g.hwFix)
            gpsSrc = "hw";
        else if (hal::gpsIsFresh())
            gpsSrc = "phone";
        char buf[320];
        snprintf(buf, sizeof(buf),
                 "{\"total\":%d,\"raven\":%d,\"ble\":\"active\","
                 "\"gps_valid\":%s,\"gps_tagged\":%d,\"gps_src\":\"%s\","
                 "\"gps_sats\":%d,\"gps_hw_detected\":%s}",
                 mod.detectionCount(), raven, hal::gpsIsFresh() ? "true" : "false", withGPS, gpsSrc,
                 g.satellites, g.hwDetected ? "true" : "false");
        r->send(200, "application/json", buf);
    });

    server.on("/api/flockyou/gps", HTTP_GET, [](AsyncWebServerRequest* r) {
        const hal::GPSData& g = hal::gpsGet();
        if (g.hwFix) {
            r->send(200, "application/json",
                    "{\"status\":\"ignored\",\"reason\":\"hw_gps_active\"}");
            return;
        }
        if (r->hasParam("lat") && r->hasParam("lon")) {
            double lat = r->getParam("lat")->value().toDouble();
            double lon = r->getParam("lon")->value().toDouble();
            float acc = r->hasParam("acc") ? r->getParam("acc")->value().toFloat() : 0;
            hal::gpsSetFromPhone(lat, lon, acc);
            r->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            r->send(400, "application/json", "{\"error\":\"lat,lon required\"}");
        }
    });

    server.on("/api/flockyou/patterns", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        AsyncResponseStream* resp = r->beginResponseStream("application/json");
        mod.writePatterns(resp);
        r->send(resp);
    });

    server.on("/api/flockyou/export/json", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        AsyncResponseStream* resp = r->beginResponseStream("application/json");
        resp->addHeader("Content-Disposition", "attachment; filename=\"flockyou_detections.json\"");
        mod.writeJSON(resp);
        r->send(resp);
    });

    server.on("/api/flockyou/export/csv", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        AsyncResponseStream* resp = r->beginResponseStream("text/csv");
        resp->addHeader("Content-Disposition", "attachment; filename=\"flockyou_detections.csv\"");
        mod.writeCSV(resp);
        r->send(resp);
    });

    server.on("/api/flockyou/export/kml", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        AsyncResponseStream* resp = r->beginResponseStream("application/vnd.google-earth.kml+xml");
        resp->addHeader("Content-Disposition", "attachment; filename=\"flockyou_detections.kml\"");
        mod.writeKML(resp);
        r->send(resp);
    });

    server.on("/api/flockyou/history", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        mod.serveHistory(r);
    });

    server.on("/api/flockyou/history/json", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        mod.serveHistoryDownload(r);
    });

    server.on("/api/flockyou/clear", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        mod.clearDetections();
        r->send(200, "application/json", "{\"status\":\"cleared\"}");
    });

    Serial.println("[FLOCKYOU] Web routes registered");
}
