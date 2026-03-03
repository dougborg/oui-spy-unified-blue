#include "skyspy.h"
#include "../hal/buzzer.h"
#include "../hal/led.h"
#include "../hal/wifi_mgr.h"
#include "odid_wifi.h"
#include "opendroneid.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// ============================================================================
// Sky Spy Module: Open Drone ID detection via BLE + WiFi promiscuous mode
// Refactored from raw/skyspy.cpp — no FreeRTOS tasks, uses shared HAL
// ============================================================================

#define SS_MAX_UAVS 8

struct SSUavData {
    uint8_t mac[6];
    int rssi;
    uint32_t lastSeen;
    char opId[ODID_ID_SIZE + 1];
    char uavId[ODID_ID_SIZE + 1];
    double latD;
    double longD;
    double baseLatD;
    double baseLongD;
    int altitudeMsl;
    int heightAgl;
    int speed;
    int heading;
    int flag;
};

static SSUavData ssUavs[SS_MAX_UAVS] = {};
static ODID_UAS_Data ssUasData;
static SemaphoreHandle_t ssMutex = NULL;
static bool ssEnabled = true;
static bool ssDeviceInRange = false;
static unsigned long ssLastHeartbeat = 0;
static unsigned long ssLastStatus = 0;

// Pointer to the module instance for the static WiFi callback
static SkySpyModule* ssInstance = nullptr;

// ============================================================================
// Helpers
// ============================================================================

static SSUavData* ssNextUav(uint8_t* mac) {
    for (int i = 0; i < SS_MAX_UAVS; i++) {
        if (memcmp(ssUavs[i].mac, mac, 6) == 0)
            return &ssUavs[i];
    }
    for (int i = 0; i < SS_MAX_UAVS; i++) {
        if (ssUavs[i].mac[0] == 0)
            return &ssUavs[i];
    }
    return &ssUavs[0];
}

static void ssSendJSON(const SSUavData* uav) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", uav->mac[0], uav->mac[1],
             uav->mac[2], uav->mac[3], uav->mac[4], uav->mac[5]);
    Serial.printf("{\"module\":\"skyspy\",\"mac\":\"%s\",\"rssi\":%d,"
                  "\"drone_lat\":%.6f,\"drone_long\":%.6f,\"drone_altitude\":%d,"
                  "\"pilot_lat\":%.6f,\"pilot_long\":%.6f,\"basic_id\":\"%s\"}\n",
                  macStr, uav->rssi, uav->latD, uav->longD, uav->altitudeMsl, uav->baseLatD,
                  uav->baseLongD, uav->uavId);
}

static void ssExtractFromODID(SSUavData* uav) {
    if (ssUasData.BasicIDValid[0]) {
        strncpy(uav->uavId, (char*)ssUasData.BasicID[0].UASID, ODID_ID_SIZE);
    }
    if (ssUasData.LocationValid) {
        uav->latD = ssUasData.Location.Latitude;
        uav->longD = ssUasData.Location.Longitude;
        uav->altitudeMsl = (int)ssUasData.Location.AltitudeGeo;
        uav->heightAgl = (int)ssUasData.Location.Height;
        uav->speed = (int)ssUasData.Location.SpeedHorizontal;
        uav->heading = (int)ssUasData.Location.Direction;
    }
    if (ssUasData.SystemValid) {
        uav->baseLatD = ssUasData.System.OperatorLatitude;
        uav->baseLongD = ssUasData.System.OperatorLongitude;
    }
    if (ssUasData.OperatorIDValid) {
        strncpy(uav->opId, (char*)ssUasData.OperatorID.OperatorId, ODID_ID_SIZE);
    }
}

static void ssTriggerDetection() {
    if (!ssDeviceInRange) {
        ssDeviceInRange = true;
        ssLastHeartbeat = millis();
        hal::buzzerPlay(hal::SND_DRONE_DETECT);
        Serial.println("[SKYSPY] Drone detected!");
    }
}

// ============================================================================
// WiFi Promiscuous Callback (static, runs in WiFi task context)
// ============================================================================

void skyspyWiFiCallback(void* buffer, wifi_promiscuous_pkt_type_t type) {
    if (!ssEnabled || type != WIFI_PKT_MGMT)
        return;

    wifi_promiscuous_pkt_t* packet = (wifi_promiscuous_pkt_t*)buffer;
    uint8_t* payload = packet->payload;
    int length = packet->rx_ctrl.sig_len;
    if (length > 4)
        length -= 4; // Strip FCS (not present in payload buffer)

    // NAN Action Frames (drone Remote ID)
    static const uint8_t nan_dest[6] = {0x51, 0x6f, 0x9a, 0x01, 0x00, 0x00};
    if (memcmp(nan_dest, &payload[4], 6) == 0) {
        char nan_mac[6] = {0};
        if (ssMutex && xSemaphoreTake(ssMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            memset(&ssUasData, 0, sizeof(ssUasData));
            if (odid_wifi_receive_message_pack_nan_action_frame(&ssUasData, nan_mac, payload,
                                                                length) == 0) {
                SSUavData uav;
                memset(&uav, 0, sizeof(uav));
                memcpy(uav.mac, &payload[10], 6);
                uav.rssi = packet->rx_ctrl.rssi;
                uav.lastSeen = millis();
                ssExtractFromODID(&uav);
                SSUavData* stored = ssNextUav(uav.mac);
                *stored = uav;
                stored->flag = 1;
                ssTriggerDetection();
                ssSendJSON(stored);
            }
            xSemaphoreGive(ssMutex);
        }
    }
    // Beacon frames with vendor-specific ODID IEs
    else if (payload[0] == 0x80) {
        int offset = 36;
        while (offset < length) {
            int typ = payload[offset];
            int len = payload[offset + 1];
            if ((typ == 0xdd) && (((payload[offset + 2] == 0x90 && payload[offset + 3] == 0x3a &&
                                    payload[offset + 4] == 0xe6)) ||
                                  ((payload[offset + 2] == 0xfa && payload[offset + 3] == 0x0b &&
                                    payload[offset + 4] == 0xbc)))) {
                int j = offset + 7;
                if (j < length) {
                    if (ssMutex && xSemaphoreTake(ssMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                        memset(&ssUasData, 0, sizeof(ssUasData));
                        odid_message_process_pack(&ssUasData, &payload[j], length - j);
                        SSUavData uav;
                        memset(&uav, 0, sizeof(uav));
                        memcpy(uav.mac, &payload[10], 6);
                        uav.rssi = packet->rx_ctrl.rssi;
                        uav.lastSeen = millis();
                        ssExtractFromODID(&uav);
                        SSUavData* stored = ssNextUav(uav.mac);
                        *stored = uav;
                        stored->flag = 1;
                        ssTriggerDetection();
                        ssSendJSON(stored);
                        xSemaphoreGive(ssMutex);
                    }
                }
            }
            offset += len + 2;
        }
    }
}

// ============================================================================
// Module Implementation
// ============================================================================

void SkySpyModule::setup() {
    ssMutex = xSemaphoreCreateMutex();
    memset(ssUavs, 0, sizeof(ssUavs));
    ssInstance = this;

    // Enable WiFi promiscuous mode for drone ID detection
    hal::wifiEnablePromiscuous(skyspyWiFiCallback, 6);
    Serial.println("[SKYSPY] Module initialized (BLE + WiFi promiscuous)");
}

void SkySpyModule::loop() {
    if (!_enabled)
        return;

    unsigned long now = millis();

    // Status heartbeat every 60s
    if (now - ssLastStatus > 60000UL) {
        Serial.println("{\"module\":\"skyspy\",\"status\":\"scanning\"}");
        ssLastStatus = now;
    }

    // Heartbeat pulse if drone in range
    if (ssDeviceInRange) {
        if (now - ssLastHeartbeat >= 5000) {
            hal::buzzerPlay(hal::SND_DRONE_HEARTBEAT);
            ssLastHeartbeat = now;
        }
        // Check if all drones out of range (7s timeout)
        bool anyDetected = false;
        if (ssMutex && xSemaphoreTake(ssMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            for (int i = 0; i < SS_MAX_UAVS; i++) {
                if (ssUavs[i].mac[0] != 0 && (now - ssUavs[i].lastSeen) < 7000) {
                    anyDetected = true;
                    break;
                }
            }
            xSemaphoreGive(ssMutex);
        }
        if (!anyDetected) {
            Serial.println("{\"module\":\"skyspy\",\"status\":\"drone_out_of_range\"}");
            ssDeviceInRange = false;
        }
    }
}

void SkySpyModule::onBLEAdvertisement(NimBLEAdvertisedDevice* device) {
    if (!_enabled)
        return;

    int len = device->getPayloadLength();
    if (len <= 5)
        return;

    uint8_t* payload = device->getPayload();
    // Check for ODID BLE service UUID signature
    if (!(payload[1] == 0x16 && payload[2] == 0xFA && payload[3] == 0xFF && payload[4] == 0x0D))
        return;

    uint8_t* mac = (uint8_t*)device->getAddress().getNative();
    if (!ssMutex || xSemaphoreTake(ssMutex, pdMS_TO_TICKS(50)) != pdTRUE)
        return;

    SSUavData* uav = ssNextUav(mac);
    uav->lastSeen = millis();
    uav->rssi = device->getRSSI();
    memcpy(uav->mac, mac, 6);

    uint8_t* odid = &payload[6];
    switch (odid[0] & 0xF0) {
    case 0x00: {
        ODID_BasicID_data basic;
        decodeBasicIDMessage(&basic, (ODID_BasicID_encoded*)odid);
        strncpy(uav->uavId, (char*)basic.UASID, ODID_ID_SIZE);
        break;
    }
    case 0x10: {
        ODID_Location_data loc;
        decodeLocationMessage(&loc, (ODID_Location_encoded*)odid);
        uav->latD = loc.Latitude;
        uav->longD = loc.Longitude;
        uav->altitudeMsl = (int)loc.AltitudeGeo;
        uav->heightAgl = (int)loc.Height;
        uav->speed = (int)loc.SpeedHorizontal;
        uav->heading = (int)loc.Direction;
        break;
    }
    case 0x40: {
        ODID_System_data sys;
        decodeSystemMessage(&sys, (ODID_System_encoded*)odid);
        uav->baseLatD = sys.OperatorLatitude;
        uav->baseLongD = sys.OperatorLongitude;
        break;
    }
    case 0x50: {
        ODID_OperatorID_data op;
        decodeOperatorIDMessage(&op, (ODID_OperatorID_encoded*)odid);
        strncpy(uav->opId, (char*)op.OperatorId, ODID_ID_SIZE);
        break;
    }
    }
    uav->flag = 1;
    ssTriggerDetection();
    ssSendJSON(uav);
    xSemaphoreGive(ssMutex);
}

// ============================================================================
// Web Routes: /api/skyspy/*
// ============================================================================

void SkySpyModule::registerRoutes(AsyncWebServer& server) {
    server.on("/api/skyspy/drones", HTTP_GET, [](AsyncWebServerRequest* r) {
        String json = "[";
        if (ssMutex && xSemaphoreTake(ssMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            bool first = true;
            for (int i = 0; i < SS_MAX_UAVS; i++) {
                if (ssUavs[i].mac[0] == 0)
                    continue;
                if (!first)
                    json += ",";
                first = false;
                char macStr[18];
                snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", ssUavs[i].mac[0],
                         ssUavs[i].mac[1], ssUavs[i].mac[2], ssUavs[i].mac[3], ssUavs[i].mac[4],
                         ssUavs[i].mac[5]);
                json += "{\"mac\":\"";
                json += macStr;
                json += "\",\"rssi\":";
                json += String(ssUavs[i].rssi);
                json += ",\"drone_lat\":";
                json += String(ssUavs[i].latD, 6);
                json += ",\"drone_long\":";
                json += String(ssUavs[i].longD, 6);
                json += ",\"altitude\":";
                json += String(ssUavs[i].altitudeMsl);
                json += ",\"height\":";
                json += String(ssUavs[i].heightAgl);
                json += ",\"speed\":";
                json += String(ssUavs[i].speed);
                json += ",\"heading\":";
                json += String(ssUavs[i].heading);
                json += ",\"pilot_lat\":";
                json += String(ssUavs[i].baseLatD, 6);
                json += ",\"pilot_long\":";
                json += String(ssUavs[i].baseLongD, 6);
                json += ",\"uav_id\":\"";
                json += ssUavs[i].uavId;
                json += "\",\"op_id\":\"";
                json += ssUavs[i].opId;
                json += "\",\"last_seen\":";
                json += String(ssUavs[i].lastSeen);
                json += "}";
            }
            xSemaphoreGive(ssMutex);
        }
        json += "]";
        r->send(200, "application/json", json);
    });

    server.on("/api/skyspy/status", HTTP_GET, [](AsyncWebServerRequest* r) {
        int count = 0;
        unsigned long now = millis();
        if (ssMutex && xSemaphoreTake(ssMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            for (int i = 0; i < SS_MAX_UAVS; i++) {
                if (ssUavs[i].mac[0] != 0 && (now - ssUavs[i].lastSeen) < 7000)
                    count++;
            }
            xSemaphoreGive(ssMutex);
        }
        char buf[100];
        snprintf(buf, sizeof(buf), "{\"active_drones\":%d,\"in_range\":%s}", count,
                 ssDeviceInRange ? "true" : "false");
        r->send(200, "application/json", buf);
    });

    Serial.println("[SKYSPY] Web routes registered");
}

bool SkySpyModule::isEnabled() {
    return _enabled;
}
void SkySpyModule::setEnabled(bool enabled) {
    _enabled = enabled;
    ssEnabled = enabled;
    if (enabled) {
        hal::wifiEnablePromiscuous(skyspyWiFiCallback, 6);
    } else {
        hal::wifiDisablePromiscuous();
        ssDeviceInRange = false;
    }
}
