#include "../modules/skyspy.h"
#include "../storage/nvs_store.h"
#include "http_helpers.h"
#include "routes.h"
#include <ArduinoJson.h>

static SkySpyModule* _ssMod = nullptr;

static esp_err_t handleSkyspyDrones(httpd_req_t* req) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    SemaphoreHandle_t mtx = _ssMod->uavMutex();
    if (mtx && xSemaphoreTake(mtx, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (int i = 0; i < SS_MAX_UAVS; i++) {
            const SSUavData& uav = _ssMod->uavAt(i);
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
    return web::sendJSON(req, 200, json.c_str());
}

static esp_err_t handleSkyspyStatus(httpd_req_t* req) {
    int count = 0;
    unsigned long now = millis();
    SemaphoreHandle_t mtx = _ssMod->uavMutex();
    if (mtx && xSemaphoreTake(mtx, pdMS_TO_TICKS(50)) == pdTRUE) {
        for (int i = 0; i < SS_MAX_UAVS; i++) {
            const SSUavData& uav = _ssMod->uavAt(i);
            if (uav.mac[0] != 0 && (now - uav.lastSeen) < 7000)
                count++;
        }
        xSemaphoreGive(mtx);
    }
    JsonDocument doc;
    doc["active_drones"] = count;
    doc["in_range"] = _ssMod->deviceInRange();
    doc["wifi_scanning"] = false; // Always false — web server only runs in normal mode
    String json;
    serializeJson(doc, json);
    return web::sendJSON(req, 200, json.c_str());
}

static esp_err_t handleStartScan(httpd_req_t* req) {
    storage::setSkyScanRequested(true);
    esp_err_t err = web::sendJSON(req, 200, "{\"ok\":true,\"reboot\":true}");
    delay(500);
    ESP.restart();
    return err;
}

void registerSkySpyRoutes(httpd_handle_t https, httpd_handle_t http, SkySpyModule& mod) {
    _ssMod = &mod;

    static const httpd_uri_t dronesUri = {"/api/skyspy/drones", HTTP_GET, handleSkyspyDrones,
                                          nullptr};
    static const httpd_uri_t statusUri = {"/api/skyspy/status", HTTP_GET, handleSkyspyStatus,
                                          nullptr};
    static const httpd_uri_t startScanUri = {"/api/skyspy/start-scan", HTTP_POST, handleStartScan,
                                             nullptr};

    web::registerOnBoth(https, http, &dronesUri);
    web::registerOnBoth(https, http, &statusUri);
    web::registerOnBoth(https, http, &startScanUri);

    Serial.println("[SKYSPY] Web routes registered");
}
