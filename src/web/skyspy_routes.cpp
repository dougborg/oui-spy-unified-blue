#include "../modules/skyspy.h"
#include "routes.h"
#include <ESPAsyncWebServer.h>

void registerSkySpyRoutes(AsyncWebServer& server, SkySpyModule& mod) {
    server.on("/api/skyspy/drones", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        String json = "[";
        SemaphoreHandle_t mtx = mod.uavMutex();
        if (mtx && xSemaphoreTake(mtx, pdMS_TO_TICKS(100)) == pdTRUE) {
            bool first = true;
            for (int i = 0; i < SS_MAX_UAVS; i++) {
                const SSUavData& uav = mod.uavAt(i);
                if (uav.mac[0] == 0)
                    continue;
                if (!first)
                    json += ",";
                first = false;
                char macStr[18];
                snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", uav.mac[0],
                         uav.mac[1], uav.mac[2], uav.mac[3], uav.mac[4], uav.mac[5]);
                json += "{\"mac\":\"";
                json += macStr;
                json += "\",\"rssi\":";
                json += String(uav.rssi);
                json += ",\"drone_lat\":";
                json += String(uav.latD, 6);
                json += ",\"drone_long\":";
                json += String(uav.longD, 6);
                json += ",\"altitude\":";
                json += String(uav.altitudeMsl);
                json += ",\"height\":";
                json += String(uav.heightAgl);
                json += ",\"speed\":";
                json += String(uav.speed);
                json += ",\"heading\":";
                json += String(uav.heading);
                json += ",\"pilot_lat\":";
                json += String(uav.baseLatD, 6);
                json += ",\"pilot_long\":";
                json += String(uav.baseLongD, 6);
                json += ",\"uav_id\":\"";
                json += uav.uavId;
                json += "\",\"op_id\":\"";
                json += uav.opId;
                json += "\",\"last_seen\":";
                json += String(uav.lastSeen);
                json += "}";
            }
            xSemaphoreGive(mtx);
        }
        json += "]";
        r->send(200, "application/json", json);
    });

    server.on("/api/skyspy/status", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        int count = 0;
        unsigned long now = millis();
        SemaphoreHandle_t mtx = mod.uavMutex();
        if (mtx && xSemaphoreTake(mtx, pdMS_TO_TICKS(50)) == pdTRUE) {
            for (int i = 0; i < SS_MAX_UAVS; i++) {
                const SSUavData& uav = mod.uavAt(i);
                if (uav.mac[0] != 0 && (now - uav.lastSeen) < 7000)
                    count++;
            }
            xSemaphoreGive(mtx);
        }
        char buf[100];
        snprintf(buf, sizeof(buf), "{\"active_drones\":%d,\"in_range\":%s}", count,
                 mod.deviceInRange() ? "true" : "false");
        r->send(200, "application/json", buf);
    });

    Serial.println("[SKYSPY] Web routes registered");
}
