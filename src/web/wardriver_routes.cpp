#include "../hal/gps.h"
#include "../modules/wardriver.h"
#include "../storage/nvs_store.h"
#include "http_helpers.h"
#include "routes.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

static WardriverModule* _wdMod = nullptr;

static esp_err_t handleWdStatus(httpd_req_t* req) {
    JsonDocument doc;
    doc["active"] = _wdMod->sessionActive();
    doc["wifi"] = _wdMod->wifiCount();
    doc["ble"] = _wdMod->bleCount();
    doc["unique"] = _wdMod->uniqueCount();
    doc["csv_size"] = (unsigned long)_wdMod->csvFileSize();
    doc["has_csv"] = _wdMod->hasCsvFile();
    doc["gps_fresh"] = hal::gpsIsFresh();
    String json;
    serializeJson(doc, json);
    return web::sendJSON(req, 200, json.c_str());
}

static esp_err_t handleWdStart(httpd_req_t* req) {
    _wdMod->startSession();
    return web::sendJSON(req, 200, "{\"ok\":true}");
}

static esp_err_t handleWdStop(httpd_req_t* req) {
    _wdMod->stopSession();
    return web::sendJSON(req, 200, "{\"ok\":true}");
}

static esp_err_t handleWdClear(httpd_req_t* req) {
    _wdMod->clearData();
    return web::sendJSON(req, 200, "{\"ok\":true}");
}

static esp_err_t handleWdRecent(httpd_req_t* req) {
    std::vector<WDRecentSighting> recent;
    _wdMod->getRecentSightings(recent);

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
    return web::sendJSON(req, 200, json.c_str());
}

static esp_err_t handleWdDownload(httpd_req_t* req) {
    if (!_wdMod->hasCsvFile()) {
        return web::sendError(req, 404, "no CSV file");
    }
    return web::sendFile(req, LittleFS, "/wigle.csv", "text/csv",
                         "attachment; filename=\"wigle.csv\"");
}

static esp_err_t handleWdConfigGet(httpd_req_t* req) {
    JsonDocument doc;
    doc["api_name"] = _wdMod->wigleApiName();
    doc["api_token"] = _wdMod->wigleApiToken().length() > 0 ? "****" : "";
    String json;
    serializeJson(doc, json);
    return web::sendJSON(req, 200, json.c_str());
}

static esp_err_t handleWdConfigPost(httpd_req_t* req) {
    char body[1024];
    if (web::readFormBody(req, body, sizeof(body)) < 0)
        return web::sendError(req, 400, "bad request");

    String apiName, apiToken;
    web::getFormValue(body, "api_name", apiName);
    web::getFormValue(body, "api_token", apiToken);
    _wdMod->setWigleConfig(apiName, apiToken);
    return web::sendJSON(req, 200, "{\"ok\":true}");
}

static esp_err_t handleWdFiltersGet(httpd_req_t* req) {
    std::vector<WDTargetFilter> filters;
    _wdMod->getFilters(filters);
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
    return web::sendJSON(req, 200, json.c_str());
}

static esp_err_t handleWdFiltersPost(httpd_req_t* req) {
    char body[4096];
    if (web::readFormBody(req, body, sizeof(body)) < 0)
        return web::sendError(req, 400, "bad request");

    String ouis, macs;
    web::getFormValue(body, "ouis", ouis);
    web::getFormValue(body, "macs", macs);
    _wdMod->parseFilters(ouis, macs);
    _wdMod->saveFilters();

    JsonDocument doc;
    doc["saved"] = _wdMod->filterCount();
    String json;
    serializeJson(doc, json);
    return web::sendJSON(req, 200, json.c_str());
}

static esp_err_t handleWdDevices(httpd_req_t* req) {
    std::vector<WDDeviceInfo> devices;
    _wdMod->getWatchlistDevices(devices);
    JsonDocument doc;
    JsonArray arr = doc["devices"].to<JsonArray>();
    unsigned long now = millis();
    for (const auto& d : devices) {
        JsonObject obj = arr.add<JsonObject>();
        obj["mac"] = d.macAddress;
        obj["rssi"] = d.rssi;
        obj["filter"] = d.filterDescription;
        obj["alias"] = _wdMod->getAlias(d.macAddress);
        unsigned long ts = (now >= d.lastSeen) ? (now - d.lastSeen) : 0;
        obj["timeSince"] = ts;
    }
    String json;
    serializeJson(doc, json);
    return web::sendJSON(req, 200, json.c_str());
}

static esp_err_t handleWdAlias(httpd_req_t* req) {
    char body[1024];
    if (web::readFormBody(req, body, sizeof(body)) < 0)
        return web::sendError(req, 400, "bad request");

    String mac, alias;
    if (!web::getFormValue(body, "mac", mac) || !web::getFormValue(body, "alias", alias))
        return web::sendError(req, 400, "missing params");

    if (alias.length() > 32)
        return web::sendError(req, 400, "alias max 32 characters");

    _wdMod->setAlias(mac, alias);
    _wdMod->saveAliases();
    return web::sendJSON(req, 200, "{\"success\":true}");
}

static esp_err_t handleWdClearDevices(httpd_req_t* req) {
    _wdMod->clearWatchlistDevices();
    return web::sendJSON(req, 200, "{\"success\":true}");
}

static esp_err_t handleWdClearFilters(httpd_req_t* req) {
    _wdMod->clearFilters();
    return web::sendJSON(req, 200, "{\"success\":true}");
}

void registerWardriverRoutes(httpd_handle_t https, httpd_handle_t http, WardriverModule& mod) {
    _wdMod = &mod;

    static const httpd_uri_t statusUri = {"/api/wardriver/status", HTTP_GET, handleWdStatus,
                                          nullptr};
    static const httpd_uri_t startUri = {"/api/wardriver/start", HTTP_POST, handleWdStart, nullptr};
    static const httpd_uri_t stopUri = {"/api/wardriver/stop", HTTP_POST, handleWdStop, nullptr};
    static const httpd_uri_t clearUri = {"/api/wardriver/clear", HTTP_POST, handleWdClear, nullptr};
    static const httpd_uri_t recentUri = {"/api/wardriver/recent", HTTP_GET, handleWdRecent,
                                          nullptr};
    static const httpd_uri_t downloadUri = {"/api/wardriver/download", HTTP_GET, handleWdDownload,
                                            nullptr};
    static const httpd_uri_t configGetUri = {"/api/wardriver/config", HTTP_GET, handleWdConfigGet,
                                             nullptr};
    static const httpd_uri_t configPostUri = {"/api/wardriver/config", HTTP_POST,
                                              handleWdConfigPost, nullptr};
    static const httpd_uri_t filtersGetUri = {"/api/wardriver/filters", HTTP_GET,
                                              handleWdFiltersGet, nullptr};
    static const httpd_uri_t filtersPostUri = {"/api/wardriver/filters", HTTP_POST,
                                               handleWdFiltersPost, nullptr};
    static const httpd_uri_t devicesUri = {"/api/wardriver/devices", HTTP_GET, handleWdDevices,
                                           nullptr};
    static const httpd_uri_t aliasUri = {"/api/wardriver/alias", HTTP_POST, handleWdAlias, nullptr};
    static const httpd_uri_t clearDevUri = {"/api/wardriver/clear-devices", HTTP_POST,
                                            handleWdClearDevices, nullptr};
    static const httpd_uri_t clearFilUri = {"/api/wardriver/clear-filters", HTTP_POST,
                                            handleWdClearFilters, nullptr};

    web::registerOnBoth(https, http, &statusUri);
    web::registerOnBoth(https, http, &startUri);
    web::registerOnBoth(https, http, &stopUri);
    web::registerOnBoth(https, http, &clearUri);
    web::registerOnBoth(https, http, &recentUri);
    web::registerOnBoth(https, http, &downloadUri);
    web::registerOnBoth(https, http, &configGetUri);
    web::registerOnBoth(https, http, &configPostUri);
    web::registerOnBoth(https, http, &filtersGetUri);
    web::registerOnBoth(https, http, &filtersPostUri);
    web::registerOnBoth(https, http, &devicesUri);
    web::registerOnBoth(https, http, &aliasUri);
    web::registerOnBoth(https, http, &clearDevUri);
    web::registerOnBoth(https, http, &clearFilUri);

    Serial.println("[WARDRIVER] Web routes registered");
}
