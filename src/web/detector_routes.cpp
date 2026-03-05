#include "../modules/detector.h"
#include "../storage/nvs_store.h"
#include "http_helpers.h"
#include "routes.h"
#include <ArduinoJson.h>

// Static module reference for handler access
static DetectorModule* _detMod = nullptr;

// Get filters
static esp_err_t handleDetFilters(httpd_req_t* req) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    const auto& filters = _detMod->filters();
    for (int i = 0; i < (int)filters.size(); i++) {
        JsonObject obj = arr.add<JsonObject>();
        obj["id"] = filters[i].identifier;
        obj["full"] = filters[i].isFullMAC;
        obj["desc"] = filters[i].description;
    }
    String json;
    serializeJson(doc, json);
    return web::sendJSON(req, 200, json.c_str());
}

// Save filters
static esp_err_t handleDetSaveFilters(httpd_req_t* req) {
    char body[4096];
    if (web::readFormBody(req, body, sizeof(body)) < 0)
        return web::sendError(req, 400, "bad request");

    String ouis, macs;
    web::getFormValue(body, "ouis", ouis);
    web::getFormValue(body, "macs", macs);

    bool buzzerEnabled = false;
    String buzzerVal;
    if (web::getFormValue(body, "buzzerEnabled", buzzerVal))
        buzzerEnabled = (buzzerVal == "true");

    bool ledEnabled = false;
    String ledVal;
    if (web::getFormValue(body, "ledEnabled", ledVal))
        ledEnabled = (ledVal == "true");

    _detMod->parseFilters(ouis, macs);
    _detMod->setBuzzerEnabled(buzzerEnabled);
    _detMod->setLedEnabled(ledEnabled);
    _detMod->saveFilters();

    JsonDocument doc;
    doc["saved"] = (int)_detMod->filters().size();
    String json;
    serializeJson(doc, json);
    return web::sendJSON(req, 200, json.c_str());
}

// Get detected devices
static esp_err_t handleDetDevices(httpd_req_t* req) {
    JsonDocument doc;
    JsonArray arr = doc["devices"].to<JsonArray>();
    unsigned long now = millis();
    const auto& devices = _detMod->devices();
    for (int i = 0; i < (int)devices.size(); i++) {
        JsonObject obj = arr.add<JsonObject>();
        obj["mac"] = devices[i].macAddress;
        obj["rssi"] = devices[i].rssi;
        obj["filter"] = devices[i].filterDescription;
        obj["alias"] = _detMod->getAlias(devices[i].macAddress);
        unsigned long ts = (now >= devices[i].lastSeen) ? (now - devices[i].lastSeen) : 0;
        obj["timeSince"] = ts;
    }
    String json;
    serializeJson(doc, json);
    return web::sendJSON(req, 200, json.c_str());
}

// Save alias
static esp_err_t handleDetAlias(httpd_req_t* req) {
    char body[512];
    if (web::readFormBody(req, body, sizeof(body)) < 0)
        return web::sendError(req, 400, "bad request");

    String mac, alias;
    if (!web::getFormValue(body, "mac", mac) || !web::getFormValue(body, "alias", alias))
        return web::sendError(req, 400, "missing params");

    if (alias.length() > 32)
        return web::sendError(req, 400, "alias max 32 characters");

    _detMod->setAlias(mac, alias);
    _detMod->saveAliases();
    return web::sendJSON(req, 200, "{\"success\":true}");
}

// Clear devices
static esp_err_t handleDetClearDevices(httpd_req_t* req) {
    _detMod->clearDevices();
    return web::sendJSON(req, 200, "{\"success\":true}");
}

// Clear filters
static esp_err_t handleDetClearFilters(httpd_req_t* req) {
    _detMod->clearFilters();
    return web::sendJSON(req, 200, "{\"success\":true}");
}

void registerDetectorRoutes(httpd_handle_t https, httpd_handle_t http, DetectorModule& mod) {
    _detMod = &mod;

    static const httpd_uri_t filtersGet = {"/api/detector/filters", HTTP_GET, handleDetFilters,
                                           nullptr};
    static const httpd_uri_t filtersPost = {"/api/detector/filters", HTTP_POST,
                                            handleDetSaveFilters, nullptr};
    static const httpd_uri_t devicesGet = {"/api/detector/devices", HTTP_GET, handleDetDevices,
                                           nullptr};
    static const httpd_uri_t aliasPost = {"/api/detector/alias", HTTP_POST, handleDetAlias,
                                          nullptr};
    static const httpd_uri_t clearDevices = {"/api/detector/clear-devices", HTTP_POST,
                                             handleDetClearDevices, nullptr};
    static const httpd_uri_t clearFilters = {"/api/detector/clear-filters", HTTP_POST,
                                             handleDetClearFilters, nullptr};

    web::registerOnBoth(https, http, &filtersGet);
    web::registerOnBoth(https, http, &filtersPost);
    web::registerOnBoth(https, http, &devicesGet);
    web::registerOnBoth(https, http, &aliasPost);
    web::registerOnBoth(https, http, &clearDevices);
    web::registerOnBoth(https, http, &clearFilters);

    Serial.println("[DETECTOR] Web routes registered");
}
