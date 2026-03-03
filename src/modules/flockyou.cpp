#include "flockyou.h"
#include "../hal/buzzer.h"
#include "../hal/neopixel.h"
#include "../hal/gps.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <NimBLEDevice.h>
#include <ctype.h>

// ============================================================================
// Flock-You Module: Surveillance device detector with all 5 detection methods
// Refactored from raw/flockyou.cpp — uses shared HAL, GPS, BLE, web server
// ============================================================================

// --- Detection Patterns (preserved from original) ---

static const char* fyMACPrefixes[] = {
    "58:8e:81", "cc:cc:cc", "ec:1b:bd", "90:35:ea", "04:0d:84",
    "f0:82:c0", "1c:34:f1", "38:5b:44", "94:34:69", "b4:e3:f9",
    "70:c9:4e", "3c:91:80", "d8:f3:bc", "80:30:49", "14:5a:fc",
    "74:4c:a1", "08:3a:88", "9c:2f:9d", "94:08:53", "e4:aa:ea"
};

static const char* fyNamePatterns[] = {
    "FS Ext Battery", "Penguin", "Flock", "Pigvision"
};

static const uint16_t fyMfrIDs[] = { 0x09C8 };

#define RAVEN_DEVICE_INFO_SERVICE   "0000180a-0000-1000-8000-00805f9b34fb"
#define RAVEN_GPS_SERVICE           "00003100-0000-1000-8000-00805f9b34fb"
#define RAVEN_POWER_SERVICE         "00003200-0000-1000-8000-00805f9b34fb"
#define RAVEN_NETWORK_SERVICE       "00003300-0000-1000-8000-00805f9b34fb"
#define RAVEN_UPLOAD_SERVICE        "00003400-0000-1000-8000-00805f9b34fb"
#define RAVEN_ERROR_SERVICE         "00003500-0000-1000-8000-00805f9b34fb"
#define RAVEN_OLD_HEALTH_SERVICE    "00001809-0000-1000-8000-00805f9b34fb"
#define RAVEN_OLD_LOCATION_SERVICE  "00001819-0000-1000-8000-00805f9b34fb"

static const char* fyRavenUUIDs[] = {
    RAVEN_DEVICE_INFO_SERVICE, RAVEN_GPS_SERVICE, RAVEN_POWER_SERVICE,
    RAVEN_NETWORK_SERVICE, RAVEN_UPLOAD_SERVICE, RAVEN_ERROR_SERVICE,
    RAVEN_OLD_HEALTH_SERVICE, RAVEN_OLD_LOCATION_SERVICE
};

// --- Detection Storage ---

#define FY_MAX_DETECTIONS 200

struct FYDetection {
    char mac[18];
    char name[48];
    int rssi;
    char method[24];
    unsigned long firstSeen;
    unsigned long lastSeen;
    int count;
    bool isRaven;
    char ravenFW[16];
    double gpsLat;
    double gpsLon;
    float gpsAcc;
    bool hasGPS;
};

static FYDetection fyDet[FY_MAX_DETECTIONS];
static int fyDetCount = 0;
static SemaphoreHandle_t fyMutex = NULL;
static bool fyEnabled = true;
static bool fyTriggered = false;
static bool fyDeviceInRange = false;
static unsigned long fyLastDetTime = 0;
static unsigned long fyLastHB = 0;

// Session persistence
#define FY_SESSION_FILE  "/session.json"
#define FY_PREV_FILE     "/prev_session.json"
#define FY_SAVE_INTERVAL 15000
static unsigned long fyLastSave = 0;
static int fyLastSaveCount = 0;
static bool fyFSReady = false;

// ============================================================================
// Detection Helpers
// ============================================================================

static bool fyCheckMAC(const uint8_t* mac) {
    char str[9];
    snprintf(str, sizeof(str), "%02x:%02x:%02x", mac[0], mac[1], mac[2]);
    for (size_t i = 0; i < sizeof(fyMACPrefixes)/sizeof(fyMACPrefixes[0]); i++) {
        if (strncasecmp(str, fyMACPrefixes[i], 8) == 0) return true;
    }
    return false;
}

static bool fyCheckName(const char* name) {
    if (!name || !name[0]) return false;
    for (size_t i = 0; i < sizeof(fyNamePatterns)/sizeof(fyNamePatterns[0]); i++) {
        if (strcasestr(name, fyNamePatterns[i])) return true;
    }
    return false;
}

static bool fyCheckMfr(uint16_t id) {
    for (size_t i = 0; i < sizeof(fyMfrIDs)/sizeof(fyMfrIDs[0]); i++) {
        if (fyMfrIDs[i] == id) return true;
    }
    return false;
}

static bool fyCheckRaven(NimBLEAdvertisedDevice* dev) {
    if (!dev || !dev->haveServiceUUID()) return false;
    int count = dev->getServiceUUIDCount();
    for (int i = 0; i < count; i++) {
        std::string str = dev->getServiceUUID(i).toString();
        for (size_t j = 0; j < sizeof(fyRavenUUIDs)/sizeof(fyRavenUUIDs[0]); j++) {
            if (strcasecmp(str.c_str(), fyRavenUUIDs[j]) == 0) return true;
        }
    }
    return false;
}

static const char* fyEstimateRavenFW(NimBLEAdvertisedDevice* dev) {
    if (!dev || !dev->haveServiceUUID()) return "?";
    bool hasNewGPS = false, hasOldLoc = false, hasPower = false;
    int count = dev->getServiceUUIDCount();
    for (int i = 0; i < count; i++) {
        std::string u = dev->getServiceUUID(i).toString();
        if (strcasecmp(u.c_str(), RAVEN_GPS_SERVICE) == 0) hasNewGPS = true;
        if (strcasecmp(u.c_str(), RAVEN_OLD_LOCATION_SERVICE) == 0) hasOldLoc = true;
        if (strcasecmp(u.c_str(), RAVEN_POWER_SERVICE) == 0) hasPower = true;
    }
    if (hasOldLoc && !hasNewGPS) return "1.1.x";
    if (hasNewGPS && !hasPower) return "1.2.x";
    if (hasNewGPS && hasPower) return "1.3.x";
    return "?";
}

static void fyAttachGPS(FYDetection& d) {
    if (hal::gpsIsFresh()) {
        const hal::GPSData& g = hal::gpsGet();
        d.hasGPS = true;
        d.gpsLat = g.lat;
        d.gpsLon = g.lon;
        d.gpsAcc = g.accuracy;
    }
}

static int fyAddDetection(const char* mac, const char* name, int rssi,
                          const char* method, bool isRaven = false,
                          const char* ravenFW = "") {
    if (!fyMutex || xSemaphoreTake(fyMutex, pdMS_TO_TICKS(100)) != pdTRUE) return -1;

    // Update existing
    for (int i = 0; i < fyDetCount; i++) {
        if (strcasecmp(fyDet[i].mac, mac) == 0) {
            fyDet[i].count++;
            fyDet[i].lastSeen = millis();
            fyDet[i].rssi = rssi;
            if (name && name[0]) strncpy(fyDet[i].name, name, sizeof(fyDet[i].name) - 1);
            fyAttachGPS(fyDet[i]);
            xSemaphoreGive(fyMutex);
            return i;
        }
    }

    // Add new
    if (fyDetCount < FY_MAX_DETECTIONS) {
        FYDetection& d = fyDet[fyDetCount];
        memset(&d, 0, sizeof(d));
        strncpy(d.mac, mac, sizeof(d.mac) - 1);
        if (name) {
            for (int j = 0; j < (int)sizeof(d.name) - 1 && name[j]; j++)
                d.name[j] = (name[j] == '"' || name[j] == '\\') ? '_' : name[j];
        }
        d.rssi = rssi;
        strncpy(d.method, method, sizeof(d.method) - 1);
        d.firstSeen = millis();
        d.lastSeen = millis();
        d.count = 1;
        d.isRaven = isRaven;
        strncpy(d.ravenFW, ravenFW ? ravenFW : "", sizeof(d.ravenFW) - 1);
        fyAttachGPS(d);
        int idx = fyDetCount++;
        xSemaphoreGive(fyMutex);
        return idx;
    }

    xSemaphoreGive(fyMutex);
    return -1;
}

// ============================================================================
// Session Persistence (LittleFS)
// ============================================================================

static void fySaveSession() {
    if (!fyFSReady || !fyMutex) return;
    if (xSemaphoreTake(fyMutex, pdMS_TO_TICKS(300)) != pdTRUE) return;
    File f = LittleFS.open(FY_SESSION_FILE, "w");
    if (!f) { xSemaphoreGive(fyMutex); return; }
    f.print("[");
    for (int i = 0; i < fyDetCount; i++) {
        if (i > 0) f.print(",");
        FYDetection& d = fyDet[i];
        f.printf("{\"mac\":\"%s\",\"name\":\"%s\",\"rssi\":%d,\"method\":\"%s\","
                 "\"first\":%lu,\"last\":%lu,\"count\":%d,\"raven\":%s,\"fw\":\"%s\"",
                 d.mac, d.name, d.rssi, d.method,
                 d.firstSeen, d.lastSeen, d.count,
                 d.isRaven ? "true" : "false", d.ravenFW);
        if (d.hasGPS) f.printf(",\"gps\":{\"lat\":%.8f,\"lon\":%.8f,\"acc\":%.1f}", d.gpsLat, d.gpsLon, d.gpsAcc);
        f.print("}");
    }
    f.print("]");
    f.close();
    fyLastSaveCount = fyDetCount;
    xSemaphoreGive(fyMutex);
}

static void fyPromotePrevSession() {
    if (!fyFSReady || !LittleFS.exists(FY_SESSION_FILE)) return;
    File src = LittleFS.open(FY_SESSION_FILE, "r");
    if (!src) return;
    String data = src.readString();
    src.close();
    if (data.length() == 0) { LittleFS.remove(FY_SESSION_FILE); return; }
    File dst = LittleFS.open(FY_PREV_FILE, "w");
    if (!dst) return;
    dst.print(data);
    dst.close();
    LittleFS.remove(FY_SESSION_FILE);
    Serial.printf("[FLOCKYOU] Prior session promoted: %d bytes\n", data.length());
}

// ============================================================================
// JSON/CSV/KML Writers
// ============================================================================

static void fyWriteJSON(AsyncResponseStream* resp) {
    resp->print("[");
    if (fyMutex && xSemaphoreTake(fyMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        for (int i = 0; i < fyDetCount; i++) {
            if (i > 0) resp->print(",");
            resp->printf("{\"mac\":\"%s\",\"name\":\"%s\",\"rssi\":%d,\"method\":\"%s\","
                "\"first\":%lu,\"last\":%lu,\"count\":%d,\"raven\":%s,\"fw\":\"%s\"",
                fyDet[i].mac, fyDet[i].name, fyDet[i].rssi, fyDet[i].method,
                fyDet[i].firstSeen, fyDet[i].lastSeen, fyDet[i].count,
                fyDet[i].isRaven ? "true" : "false", fyDet[i].ravenFW);
            if (fyDet[i].hasGPS) resp->printf(",\"gps\":{\"lat\":%.8f,\"lon\":%.8f,\"acc\":%.1f}",
                fyDet[i].gpsLat, fyDet[i].gpsLon, fyDet[i].gpsAcc);
            resp->print("}");
        }
        xSemaphoreGive(fyMutex);
    }
    resp->print("]");
}

static void fyWriteKML(AsyncResponseStream* resp) {
    resp->print("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n<Document>\n"
                "<name>Flock-You Detections</name>\n"
                "<Style id=\"det\"><IconStyle><color>ff4489ec</color><scale>1.0</scale></IconStyle></Style>\n"
                "<Style id=\"raven\"><IconStyle><color>ff4444ef</color><scale>1.2</scale></IconStyle></Style>\n");
    if (fyMutex && xSemaphoreTake(fyMutex, pdMS_TO_TICKS(300)) == pdTRUE) {
        for (int i = 0; i < fyDetCount; i++) {
            FYDetection& d = fyDet[i];
            if (!d.hasGPS) continue;
            resp->printf("<Placemark><name>%s</name><styleUrl>#%s</styleUrl>"
                "<description><![CDATA[", d.mac, d.isRaven ? "raven" : "det");
            if (d.name[0]) resp->printf("<b>Name:</b> %s<br/>", d.name);
            resp->printf("<b>Method:</b> %s<br/><b>RSSI:</b> %d<br/><b>Count:</b> %d",
                d.method, d.rssi, d.count);
            if (d.isRaven) resp->printf("<br/><b>FW:</b> %s", d.ravenFW);
            resp->printf("]]></description><Point><coordinates>%.8f,%.8f,0</coordinates></Point></Placemark>\n",
                d.gpsLon, d.gpsLat);
        }
        xSemaphoreGive(fyMutex);
    }
    resp->print("</Document>\n</kml>");
}

// ============================================================================
// Module Implementation
// ============================================================================

void FlockyouModule::setup() {
    fyMutex = xSemaphoreCreateMutex();

    if (LittleFS.begin(true)) {
        fyFSReady = true;
        fyPromotePrevSession();
        Serial.println("[FLOCKYOU] LittleFS ready");
    }

    hal::neopixelSetIdleHue(270); // Purple for flockyou
    Serial.println("[FLOCKYOU] Module initialized");
}

void FlockyouModule::loop() {
    if (!_enabled) return;

    // Heartbeat tracking
    if (fyDeviceInRange) {
        if (millis() - fyLastHB >= 10000) {
            hal::buzzerPlay(hal::SND_CROW_HEARTBEAT);
            hal::neopixelSetState(hal::NEO_HEARTBEAT_GLOW, 300);
            fyLastHB = millis();
        }
        if (millis() - fyLastDetTime >= 30000) {
            fyDeviceInRange = false;
            fyTriggered = false;
            hal::neopixelSetState(hal::NEO_IDLE_BREATHING);
        }
    }

    // Auto-save
    if (fyFSReady && millis() - fyLastSave >= FY_SAVE_INTERVAL) {
        if (fyDetCount > 0 && fyDetCount != fyLastSaveCount) fySaveSession();
        fyLastSave = millis();
    } else if (fyFSReady && fyDetCount > 0 && fyLastSaveCount == 0 &&
               millis() - fyLastSave >= 5000) {
        fySaveSession();
        fyLastSave = millis();
    }
}

void FlockyouModule::onBLEAdvertisement(NimBLEAdvertisedDevice* dev) {
    if (!_enabled) return;

    NimBLEAddress addr = dev->getAddress();
    std::string addrStr = addr.toString();

    unsigned int m[6];
    sscanf(addrStr.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",
           &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]);
    uint8_t mac[6] = {(uint8_t)m[0], (uint8_t)m[1], (uint8_t)m[2],
                      (uint8_t)m[3], (uint8_t)m[4], (uint8_t)m[5]};

    int rssi = dev->getRSSI();
    std::string name = dev->haveName() ? dev->getName() : "";

    bool detected = false;
    const char* method = "";
    bool isRaven = false;
    const char* ravenFW = "";

    // 1. MAC prefix
    if (fyCheckMAC(mac)) { detected = true; method = "mac_prefix"; }

    // 2. Device name
    if (!detected && !name.empty() && fyCheckName(name.c_str())) { detected = true; method = "device_name"; }

    // 3. Manufacturer ID
    if (!detected) {
        for (uint8_t i = 0; i < dev->getManufacturerDataCount(); i++) {
            std::string data = dev->getManufacturerData(i);
            if (data.size() >= 2) {
                uint16_t code = ((uint16_t)(uint8_t)data[1] << 8) | (uint16_t)(uint8_t)data[0];
                if (fyCheckMfr(code)) { detected = true; method = "ble_mfr_id"; break; }
            }
        }
    }

    // 4. Raven UUID
    if (!detected && fyCheckRaven(dev)) {
        detected = true; method = "raven_uuid"; isRaven = true;
        ravenFW = fyEstimateRavenFW(dev);
    }

    if (!detected) return;

    int idx = fyAddDetection(addrStr.c_str(), name.c_str(), rssi, method, isRaven, ravenFW);

    // Serial JSON output
    char gpsBuf[80] = "";
    if (hal::gpsIsFresh()) {
        const hal::GPSData& g = hal::gpsGet();
        snprintf(gpsBuf, sizeof(gpsBuf), ",\"gps\":{\"latitude\":%.8f,\"longitude\":%.8f,\"accuracy\":%.1f}",
                 g.lat, g.lon, g.accuracy);
    }
    if (isRaven) {
        Serial.printf("{\"module\":\"flockyou\",\"detection_method\":\"%s\",\"mac_address\":\"%s\","
               "\"device_name\":\"%s\",\"rssi\":%d,\"is_raven\":true,\"raven_fw\":\"%s\"%s}\n",
               method, addrStr.c_str(), name.c_str(), rssi, ravenFW, gpsBuf);
    } else {
        Serial.printf("{\"module\":\"flockyou\",\"detection_method\":\"%s\",\"mac_address\":\"%s\","
               "\"device_name\":\"%s\",\"rssi\":%d%s}\n",
               method, addrStr.c_str(), name.c_str(), rssi, gpsBuf);
    }

    if (!fyTriggered) {
        fyTriggered = true;
        hal::buzzerPlay(hal::SND_CROW_ALARM);
        hal::neopixelFlash(0, 300, 0); // red-pink-red
    }
    fyDeviceInRange = true;
    fyLastDetTime = millis();
    fyLastHB = millis();
}

// ============================================================================
// Web Routes: /api/flockyou/*
// ============================================================================

void FlockyouModule::registerRoutes(AsyncWebServer& server) {
    server.on("/api/flockyou/detections", HTTP_GET, [](AsyncWebServerRequest* r) {
        AsyncResponseStream* resp = r->beginResponseStream("application/json");
        fyWriteJSON(resp);
        r->send(resp);
    });

    server.on("/api/flockyou/stats", HTTP_GET, [](AsyncWebServerRequest* r) {
        int raven = 0, withGPS = 0;
        if (fyMutex && xSemaphoreTake(fyMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            for (int i = 0; i < fyDetCount; i++) {
                if (fyDet[i].isRaven) raven++;
                if (fyDet[i].hasGPS) withGPS++;
            }
            xSemaphoreGive(fyMutex);
        }
        const hal::GPSData& g = hal::gpsGet();
        const char* gpsSrc = "none";
        if (g.isHardware && g.hwFix) gpsSrc = "hw";
        else if (hal::gpsIsFresh()) gpsSrc = "phone";
        char buf[320];
        snprintf(buf, sizeof(buf),
            "{\"total\":%d,\"raven\":%d,\"ble\":\"active\","
            "\"gps_valid\":%s,\"gps_tagged\":%d,\"gps_src\":\"%s\","
            "\"gps_sats\":%d,\"gps_hw_detected\":%s}",
            fyDetCount, raven,
            hal::gpsIsFresh() ? "true" : "false",
            withGPS, gpsSrc, g.satellites,
            g.hwDetected ? "true" : "false");
        r->send(200, "application/json", buf);
    });

    server.on("/api/flockyou/gps", HTTP_GET, [](AsyncWebServerRequest* r) {
        const hal::GPSData& g = hal::gpsGet();
        if (g.hwFix) {
            r->send(200, "application/json", "{\"status\":\"ignored\",\"reason\":\"hw_gps_active\"}");
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

    server.on("/api/flockyou/patterns", HTTP_GET, [](AsyncWebServerRequest* r) {
        AsyncResponseStream* resp = r->beginResponseStream("application/json");
        resp->print("{\"macs\":[");
        for (size_t i = 0; i < sizeof(fyMACPrefixes)/sizeof(fyMACPrefixes[0]); i++) {
            if (i > 0) resp->print(",");
            resp->printf("\"%s\"", fyMACPrefixes[i]);
        }
        resp->print("],\"names\":[");
        for (size_t i = 0; i < sizeof(fyNamePatterns)/sizeof(fyNamePatterns[0]); i++) {
            if (i > 0) resp->print(",");
            resp->printf("\"%s\"", fyNamePatterns[i]);
        }
        resp->print("],\"mfr\":[");
        for (size_t i = 0; i < sizeof(fyMfrIDs)/sizeof(fyMfrIDs[0]); i++) {
            if (i > 0) resp->print(",");
            resp->printf("%u", fyMfrIDs[i]);
        }
        resp->print("],\"raven\":[");
        for (size_t i = 0; i < sizeof(fyRavenUUIDs)/sizeof(fyRavenUUIDs[0]); i++) {
            if (i > 0) resp->print(",");
            resp->printf("\"%s\"", fyRavenUUIDs[i]);
        }
        resp->print("]}");
        r->send(resp);
    });

    server.on("/api/flockyou/export/json", HTTP_GET, [](AsyncWebServerRequest* r) {
        AsyncResponseStream* resp = r->beginResponseStream("application/json");
        resp->addHeader("Content-Disposition", "attachment; filename=\"flockyou_detections.json\"");
        fyWriteJSON(resp);
        r->send(resp);
    });

    server.on("/api/flockyou/export/csv", HTTP_GET, [](AsyncWebServerRequest* r) {
        AsyncResponseStream* resp = r->beginResponseStream("text/csv");
        resp->addHeader("Content-Disposition", "attachment; filename=\"flockyou_detections.csv\"");
        resp->println("mac,name,rssi,method,first_seen_ms,last_seen_ms,count,is_raven,raven_fw,latitude,longitude,gps_accuracy");
        if (fyMutex && xSemaphoreTake(fyMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
            for (int i = 0; i < fyDetCount; i++) {
                FYDetection& d = fyDet[i];
                if (d.hasGPS) {
                    resp->printf("\"%s\",\"%s\",%d,\"%s\",%lu,%lu,%d,%s,\"%s\",%.8f,%.8f,%.1f\n",
                        d.mac, d.name, d.rssi, d.method, d.firstSeen, d.lastSeen, d.count,
                        d.isRaven ? "true" : "false", d.ravenFW, d.gpsLat, d.gpsLon, d.gpsAcc);
                } else {
                    resp->printf("\"%s\",\"%s\",%d,\"%s\",%lu,%lu,%d,%s,\"%s\",,,\n",
                        d.mac, d.name, d.rssi, d.method, d.firstSeen, d.lastSeen, d.count,
                        d.isRaven ? "true" : "false", d.ravenFW);
                }
            }
            xSemaphoreGive(fyMutex);
        }
        r->send(resp);
    });

    server.on("/api/flockyou/export/kml", HTTP_GET, [](AsyncWebServerRequest* r) {
        AsyncResponseStream* resp = r->beginResponseStream("application/vnd.google-earth.kml+xml");
        resp->addHeader("Content-Disposition", "attachment; filename=\"flockyou_detections.kml\"");
        fyWriteKML(resp);
        r->send(resp);
    });

    server.on("/api/flockyou/history", HTTP_GET, [](AsyncWebServerRequest* r) {
        if (fyFSReady && LittleFS.exists(FY_PREV_FILE)) {
            r->send(LittleFS, FY_PREV_FILE, "application/json");
        } else {
            r->send(200, "application/json", "[]");
        }
    });

    server.on("/api/flockyou/history/json", HTTP_GET, [](AsyncWebServerRequest* r) {
        if (fyFSReady && LittleFS.exists(FY_PREV_FILE)) {
            AsyncWebServerResponse* resp = r->beginResponse(LittleFS, FY_PREV_FILE, "application/json");
            resp->addHeader("Content-Disposition", "attachment; filename=\"flockyou_prev_session.json\"");
            r->send(resp);
        } else {
            r->send(404, "application/json", "{\"error\":\"no prior session\"}");
        }
    });

    server.on("/api/flockyou/clear", HTTP_GET, [](AsyncWebServerRequest* r) {
        fySaveSession();
        if (fyMutex && xSemaphoreTake(fyMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
            fyDetCount = 0;
            memset(fyDet, 0, sizeof(fyDet));
            fyTriggered = false;
            fyDeviceInRange = false;
            xSemaphoreGive(fyMutex);
        }
        r->send(200, "application/json", "{\"status\":\"cleared\"}");
    });

    Serial.println("[FLOCKYOU] Web routes registered");
}

bool FlockyouModule::isEnabled() { return _enabled; }
void FlockyouModule::setEnabled(bool enabled) {
    _enabled = enabled;
    fyEnabled = enabled;
}
