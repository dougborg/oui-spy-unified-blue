#include "../modules/detector_logic.h"
#include "../modules/foxhunter.h"
#include "http_helpers.h"
#include "routes.h"
#include <ArduinoJson.h>

static FoxhunterModule* _foxMod = nullptr;

// Get current target and RSSI
static esp_err_t handleFoxStatus(httpd_req_t* req) {
    JsonDocument doc;
    doc["target"] = _foxMod->targetMAC();
    doc["detected"] = _foxMod->targetDetected();
    doc["rssi"] = _foxMod->currentRSSI();
    doc["lastSeen"] = _foxMod->lastTargetSeen();
    String json;
    serializeJson(doc, json);
    return web::sendJSON(req, 200, json.c_str());
}

// Set target MAC
static esp_err_t handleFoxSetTarget(httpd_req_t* req) {
    char body[128];
    if (web::readFormBody(req, body, sizeof(body)) < 0)
        return web::sendError(req, 400, "bad request");

    String mac;
    if (!web::getFormValue(body, "mac", mac))
        return web::sendError(req, 400, "missing mac");

    mac.trim();
    if (mac.length() > 0 && !detector_logic::isValidMac(mac.c_str()))
        return web::sendError(req, 400, "invalid MAC format");

    _foxMod->setTarget(mac);
    JsonDocument doc;
    doc["target"] = _foxMod->targetMAC();
    String json;
    serializeJson(doc, json);
    return web::sendJSON(req, 200, json.c_str());
}

// Get live RSSI
static esp_err_t handleFoxRSSI(httpd_req_t* req) {
    JsonDocument doc;
    doc["rssi"] = _foxMod->currentRSSI();
    doc["detected"] = _foxMod->targetDetected();
    String json;
    serializeJson(doc, json);
    return web::sendJSON(req, 200, json.c_str());
}

// Clear target
static esp_err_t handleFoxClear(httpd_req_t* req) {
    _foxMod->clearTarget();
    return web::sendJSON(req, 200, "{\"cleared\":true}");
}

void registerFoxhunterRoutes(httpd_handle_t https, httpd_handle_t http, FoxhunterModule& mod) {
    _foxMod = &mod;

    static const httpd_uri_t statusUri = {"/api/foxhunter/status", HTTP_GET, handleFoxStatus,
                                          nullptr};
    static const httpd_uri_t targetUri = {"/api/foxhunter/target", HTTP_POST, handleFoxSetTarget,
                                          nullptr};
    static const httpd_uri_t rssiUri = {"/api/foxhunter/rssi", HTTP_GET, handleFoxRSSI, nullptr};
    static const httpd_uri_t clearUri = {"/api/foxhunter/clear", HTTP_POST, handleFoxClear, nullptr};

    web::registerOnBoth(https, http, &statusUri);
    web::registerOnBoth(https, http, &targetUri);
    web::registerOnBoth(https, http, &rssiUri);
    web::registerOnBoth(https, http, &clearUri);

    Serial.println("[FOXHUNTER] Web routes registered");
}
