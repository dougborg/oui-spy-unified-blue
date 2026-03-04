#include "../modules/skyspy.h"
#include "routes.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

void registerSkySpyRoutes(AsyncWebServer& server, SkySpyModule& mod) {
    server.on("/api/skyspy/drones", HTTP_GET, [&mod](AsyncWebServerRequest* r) {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        SemaphoreHandle_t mtx = mod.uavMutex();
        if (mtx && xSemaphoreTake(mtx, pdMS_TO_TICKS(100)) == pdTRUE) {
            for (int i = 0; i < SS_MAX_UAVS; i++) {
                const SSUavData& uav = mod.uavAt(i);
                if (uav.mac[0] == 0)
                    continue;
                JsonObject obj = arr.add<JsonObject>();
                char macStr[18];
                snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", uav.mac[0],
                         uav.mac[1], uav.mac[2], uav.mac[3], uav.mac[4], uav.mac[5]);
                obj["mac"] = macStr;
                obj["rssi"] = uav.rssi;
                obj["drone_lat"] = serialized(String(uav.latD, 6));
                obj["drone_long"] = serialized(String(uav.longD, 6));
                obj["altitude"] = uav.altitudeMsl;
                obj["height"] = uav.heightAgl;
                obj["speed"] = uav.speed;
                obj["heading"] = uav.heading;
                obj["pilot_lat"] = serialized(String(uav.baseLatD, 6));
                obj["pilot_long"] = serialized(String(uav.baseLongD, 6));
                obj["uav_id"] = uav.uavId;
                obj["op_id"] = uav.opId;
                obj["last_seen"] = uav.lastSeen;
            }
            xSemaphoreGive(mtx);
        }
        String json;
        serializeJson(doc, json);
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
        JsonDocument doc;
        doc["active_drones"] = count;
        doc["in_range"] = mod.deviceInRange();
        String json;
        serializeJson(doc, json);
        r->send(200, "application/json", json);
    });

    Serial.println("[SKYSPY] Web routes registered");
}
