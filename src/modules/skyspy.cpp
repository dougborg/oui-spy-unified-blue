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
// ============================================================================

// Pointer to the module instance for the static WiFi callback
static SkySpyModule* ssInstance = nullptr;

// ============================================================================
// Helpers
// ============================================================================

SSUavData* SkySpyModule::nextUav(uint8_t* mac) {
    for (int i = 0; i < SS_MAX_UAVS; i++) {
        if (memcmp(_uavs[i].mac, mac, 6) == 0)
            return &_uavs[i];
    }
    for (int i = 0; i < SS_MAX_UAVS; i++) {
        if (_uavs[i].mac[0] == 0)
            return &_uavs[i];
    }
    return &_uavs[0];
}

void SkySpyModule::sendJSON(const SSUavData* uav) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", uav->mac[0], uav->mac[1],
             uav->mac[2], uav->mac[3], uav->mac[4], uav->mac[5]);
    Serial.printf("{\"module\":\"skyspy\",\"mac\":\"%s\",\"rssi\":%d,"
                  "\"drone_lat\":%.6f,\"drone_long\":%.6f,\"drone_altitude\":%d,"
                  "\"pilot_lat\":%.6f,\"pilot_long\":%.6f,\"basic_id\":\"%s\"}\n",
                  macStr, uav->rssi, uav->latD, uav->longD, uav->altitudeMsl, uav->baseLatD,
                  uav->baseLongD, uav->uavId);
}

void SkySpyModule::extractFromODID(SSUavData* uav) {
    if (_uasData.BasicIDValid[0]) {
        strncpy(uav->uavId, (char*)_uasData.BasicID[0].UASID, ODID_ID_SIZE);
    }
    if (_uasData.LocationValid) {
        uav->latD = _uasData.Location.Latitude;
        uav->longD = _uasData.Location.Longitude;
        uav->altitudeMsl = (int)_uasData.Location.AltitudeGeo;
        uav->heightAgl = (int)_uasData.Location.Height;
        uav->speed = (int)_uasData.Location.SpeedHorizontal;
        uav->heading = (int)_uasData.Location.Direction;
    }
    if (_uasData.SystemValid) {
        uav->baseLatD = _uasData.System.OperatorLatitude;
        uav->baseLongD = _uasData.System.OperatorLongitude;
    }
    if (_uasData.OperatorIDValid) {
        strncpy(uav->opId, (char*)_uasData.OperatorID.OperatorId, ODID_ID_SIZE);
    }
}

void SkySpyModule::triggerDetection() {
    if (!_deviceInRange) {
        _deviceInRange = true;
        _lastHeartbeat = millis();
        hal::buzzerPlay(hal::SND_DRONE_DETECT);
        Serial.println("[SKYSPY] Drone detected!");
    }
}

// ============================================================================
// WiFi Promiscuous Callback (static, runs in WiFi task context)
// ============================================================================

void skyspyWiFiCallback(void* buffer, wifi_promiscuous_pkt_type_t type) {
    if (!ssInstance || type != WIFI_PKT_MGMT)
        return;

    wifi_promiscuous_pkt_t* packet = (wifi_promiscuous_pkt_t*)buffer;
    uint8_t* payload = packet->payload;
    int length = packet->rx_ctrl.sig_len;
    if (length > 4)
        length -= 4; // Strip FCS (not present in payload buffer)

    ssInstance->handleWiFiFrame(payload, length, packet->rx_ctrl.rssi);
}

void SkySpyModule::handleWiFiFrame(const uint8_t* payload, int length, int rssi) {
    if (!_enabled)
        return;

    // NAN Action Frames (drone Remote ID)
    static const uint8_t nan_dest[6] = {0x51, 0x6f, 0x9a, 0x01, 0x00, 0x00};
    if (length >= 10 && memcmp(nan_dest, &payload[4], 6) == 0) {
        char nan_mac[6] = {0};
        if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            memset(&_uasData, 0, sizeof(_uasData));
            if (odid_wifi_receive_message_pack_nan_action_frame(&_uasData, nan_mac,
                                                                (uint8_t*)payload, length) == 0) {
                SSUavData uav;
                memset(&uav, 0, sizeof(uav));
                memcpy(uav.mac, &payload[10], 6);
                uav.rssi = rssi;
                uav.lastSeen = millis();
                extractFromODID(&uav);
                SSUavData* stored = nextUav(uav.mac);
                *stored = uav;
                stored->flag = 1;
                triggerDetection();
                sendJSON(stored);
            }
            xSemaphoreGive(_mutex);
        }
    }
    // Beacon frames with vendor-specific ODID IEs
    else if (payload[0] == 0x80) {
        int offset = 36;
        while (offset + 2 <= length) {
            int typ = payload[offset];
            int len = payload[offset + 1];
            if (offset + 2 + len > length)
                break;
            if ((typ == 0xdd) && (len >= 5) &&
                (((payload[offset + 2] == 0x90 && payload[offset + 3] == 0x3a &&
                   payload[offset + 4] == 0xe6)) ||
                 ((payload[offset + 2] == 0xfa && payload[offset + 3] == 0x0b &&
                   payload[offset + 4] == 0xbc)))) {
                int j = offset + 7;
                if (j < length) {
                    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                        memset(&_uasData, 0, sizeof(_uasData));
                        odid_message_process_pack(&_uasData, (uint8_t*)&payload[j], length - j);
                        SSUavData uav;
                        memset(&uav, 0, sizeof(uav));
                        memcpy(uav.mac, &payload[10], 6);
                        uav.rssi = rssi;
                        uav.lastSeen = millis();
                        extractFromODID(&uav);
                        SSUavData* stored = nextUav(uav.mac);
                        *stored = uav;
                        stored->flag = 1;
                        triggerDetection();
                        sendJSON(stored);
                        xSemaphoreGive(_mutex);
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
    _mutex = xSemaphoreCreateMutex();
    memset(_uavs, 0, sizeof(_uavs));
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
    if (now - _lastStatus > 60000UL) {
        Serial.println("{\"module\":\"skyspy\",\"status\":\"scanning\"}");
        _lastStatus = now;
    }

    // Heartbeat pulse if drone in range
    if (_deviceInRange) {
        if (now - _lastHeartbeat >= 5000) {
            hal::buzzerPlay(hal::SND_DRONE_HEARTBEAT);
            _lastHeartbeat = now;
        }
        // Check if all drones out of range (7s timeout)
        bool anyDetected = false;
        if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            for (int i = 0; i < SS_MAX_UAVS; i++) {
                if (_uavs[i].mac[0] != 0 && (now - _uavs[i].lastSeen) < 7000) {
                    anyDetected = true;
                    break;
                }
            }
            xSemaphoreGive(_mutex);
        }
        if (!anyDetected) {
            Serial.println("{\"module\":\"skyspy\",\"status\":\"drone_out_of_range\"}");
            _deviceInRange = false;
        }
    }
}

void SkySpyModule::onBLEAdvertisement(NimBLEAdvertisedDevice* device) {
    if (!_enabled)
        return;

    int len = device->getPayloadLength();
    if (len < 6)
        return;

    uint8_t* payload = device->getPayload();
    // Check for ODID BLE service UUID signature
    if (!(payload[1] == 0x16 && payload[2] == 0xFA && payload[3] == 0xFF && payload[4] == 0x0D))
        return;

    uint8_t* mac = (uint8_t*)device->getAddress().getNative();
    if (!_mutex || xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)) != pdTRUE)
        return;

    SSUavData* uav = nextUav(mac);
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
    triggerDetection();
    sendJSON(uav);
    xSemaphoreGive(_mutex);
}

// ============================================================================
// Web Routes: /api/skyspy/*
// ============================================================================

void SkySpyModule::registerRoutes(AsyncWebServer& server) {
    server.on("/api/skyspy/drones", HTTP_GET, [this](AsyncWebServerRequest* r) {
        String json = "[";
        if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            bool first = true;
            for (int i = 0; i < SS_MAX_UAVS; i++) {
                if (_uavs[i].mac[0] == 0)
                    continue;
                if (!first)
                    json += ",";
                first = false;
                char macStr[18];
                snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", _uavs[i].mac[0],
                         _uavs[i].mac[1], _uavs[i].mac[2], _uavs[i].mac[3], _uavs[i].mac[4],
                         _uavs[i].mac[5]);
                json += "{\"mac\":\"";
                json += macStr;
                json += "\",\"rssi\":";
                json += String(_uavs[i].rssi);
                json += ",\"drone_lat\":";
                json += String(_uavs[i].latD, 6);
                json += ",\"drone_long\":";
                json += String(_uavs[i].longD, 6);
                json += ",\"altitude\":";
                json += String(_uavs[i].altitudeMsl);
                json += ",\"height\":";
                json += String(_uavs[i].heightAgl);
                json += ",\"speed\":";
                json += String(_uavs[i].speed);
                json += ",\"heading\":";
                json += String(_uavs[i].heading);
                json += ",\"pilot_lat\":";
                json += String(_uavs[i].baseLatD, 6);
                json += ",\"pilot_long\":";
                json += String(_uavs[i].baseLongD, 6);
                json += ",\"uav_id\":\"";
                json += _uavs[i].uavId;
                json += "\",\"op_id\":\"";
                json += _uavs[i].opId;
                json += "\",\"last_seen\":";
                json += String(_uavs[i].lastSeen);
                json += "}";
            }
            xSemaphoreGive(_mutex);
        }
        json += "]";
        r->send(200, "application/json", json);
    });

    server.on("/api/skyspy/status", HTTP_GET, [this](AsyncWebServerRequest* r) {
        int count = 0;
        unsigned long now = millis();
        if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            for (int i = 0; i < SS_MAX_UAVS; i++) {
                if (_uavs[i].mac[0] != 0 && (now - _uavs[i].lastSeen) < 7000)
                    count++;
            }
            xSemaphoreGive(_mutex);
        }
        char buf[100];
        snprintf(buf, sizeof(buf), "{\"active_drones\":%d,\"in_range\":%s}", count,
                 _deviceInRange ? "true" : "false");
        r->send(200, "application/json", buf);
    });

    Serial.println("[SKYSPY] Web routes registered");
}

bool SkySpyModule::isEnabled() {
    return _enabled;
}
void SkySpyModule::setEnabled(bool enabled) {
    _enabled = enabled;
    if (enabled) {
        hal::wifiEnablePromiscuous(skyspyWiFiCallback, 6);
    } else {
        hal::wifiDisablePromiscuous();
        _deviceInRange = false;
    }
}
