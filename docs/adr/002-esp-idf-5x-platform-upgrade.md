# ADR-002: Upgrade to ESP-IDF 5.x (Arduino-ESP32 v3.x)

**Status:** Accepted
**Date:** 2026-03-04
**Decision makers:** colonelpanichacks

## Problem Statement

The project uses `espressif32@^6.3.0` (resolves to 6.13.0), which bundles **Arduino-ESP32 v2.0.17** based on **ESP-IDF 4.4.7**. This is the end of the 4.4 line — Espressif's active development targets ESP-IDF 5.x exclusively. Staying on 4.4 means no new features, no security patches beyond LTS, and increasing library incompatibility.

The immediate symptom: `httpd_ssl_config_t` lacks the `servercert` field (added in ESP-IDF 5.0), forcing us to overload the `cacert_pem` field for the server certificate — a confusing workaround documented in `server.cpp`.

## Current Platform Stack

| Component | Current Version | Latest Available |
|-----------|----------------|------------------|
| PlatformIO platform | espressif32 6.13.0 | pioarduino 55.03.37 |
| Arduino-ESP32 core | v2.0.17 | v3.3.7 |
| ESP-IDF | 4.4.7 | 5.5.2 |
| NimBLE-Arduino | 1.4.3 | 2.3.8 |

## The Platform Situation

**The official PlatformIO espressif32 platform does NOT support Arduino-ESP32 v3.x.** Espressif and PlatformIO parted ways — PlatformIO's official platform still ships Arduino-ESP32 v2.x and will not adopt v3.x. The only path to ESP-IDF 5.x with Arduino framework is the **pioarduino** community fork.

This is the single biggest risk factor in the upgrade.

---

## Breaking Changes Inventory

### HIGH — NimBLE-Arduino 1.4 → 2.x

NimBLE-Arduino 1.4.x **does not compile** against ESP-IDF 5.x (`esp_nimble_hci_and_controller_init()` was removed). Must upgrade to 2.x.

**Affected files:** `src/hal/ble_mgr.cpp`, `src/hal/ble_mgr.h`, + all 5 module `onBLEAdvertisement()` implementations

| Change | Before (1.x) | After (2.x) |
|--------|--------------|-------------|
| Callback base class | `NimBLEAdvertisedDeviceCallbacks` | `NimBLEScanCallbacks` |
| Callback method | `onResult(NimBLEAdvertisedDevice*)` | `onResult(const NimBLEAdvertisedDevice*)` |
| Register callbacks | `setAdvertisedDeviceCallbacks(&cb, false)` | `setScanCallbacks(&cb, false)` |
| Set TX power | `setPower(ESP_PWR_LVL_P9)` | `setPower(9)` (int8_t dBm) |
| Scan duration | `start(3, nullptr, false)` (seconds) | `start(3000, false)` (milliseconds) |
| BLEListener interface | `onBLEAdvertisement(NimBLEAdvertisedDevice*)` | `onBLEAdvertisement(const NimBLEAdvertisedDevice*)` |

The const pointer change cascades to all 5 modules: detector, foxhunter, flockyou, skyspy, wardriver.

### HIGH — DHCP Server API

`dhcps_set_option_info()` and `dhcps_offer_t` are removed in ESP-IDF 5.x.

**Affected file:** `src/hal/wifi_mgr.cpp` (lines 2-3, 36-37)

```cpp
// BEFORE (ESP-IDF 4.4):
#include <dhcpserver/dhcpserver.h>
#include <dhcpserver/dhcpserver_options.h>
dhcps_offer_t offer = OFFER_DNS;
dhcps_set_option_info(6, &offer, sizeof(offer));

// AFTER (ESP-IDF 5.x):
#include <esp_netif.h>
esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
uint8_t offer = 1;
esp_netif_dhcps_option(netif, ESP_NETIF_OP_SET,
                       ESP_NETIF_DOMAIN_NAME_SERVER, &offer, sizeof(offer));
```

### HIGH — HTTPS Server Certificate Config

The `cacert_pem` / `servercert` field split is the original motivation for this ADR.

**Affected file:** `src/web/server.cpp` (lines 199-206)

```cpp
// BEFORE (ESP-IDF 4.4) — cacert_pem overloaded as server cert:
httpsConfig.cacert_pem = DEV_CERT_PEM;
httpsConfig.cacert_len = DEV_CERT_PEM_LEN;

// AFTER (ESP-IDF 5.x) — dedicated servercert field:
httpsConfig.servercert = DEV_CERT_PEM;
httpsConfig.servercert_len = DEV_CERT_PEM_LEN;
// cacert_pem now correctly reserved for mutual TLS client verification
```

### MEDIUM — Build Flags

| Flag | Action |
|------|--------|
| `-mfix-esp32-psram-cache-issue` | **Remove.** This is a workaround for an original ESP32 silicon bug. ESP32-S3 does not have this bug — it was never needed. |
| `-DBOARD_HAS_PSRAM` | Keep. pioarduino board definition handles PSRAM config, but explicit flag is harmless. |
| `-DARDUINO_USB_CDC_ON_BOOT=1` | Keep. Still honored in v3.x. |
| `-DCONFIG_BT_NIMBLE_ENABLED=1` | Keep. NimBLE is default in v3.x, but explicit is clearer. |
| `-DCONFIG_ESP_HTTPS_SERVER_ENABLE=1` | Keep. Still needed. |
| `-Werror` | Keep, but expect new warnings from ESP-IDF 5.x headers to address. |

### LOW — Stable APIs (no changes expected)

These APIs are used extensively but remain compatible:

- **WiFi** — `WiFi.mode()`, `WiFi.softAP()`, `WiFi.softAPConfig()`, `WiFi.softAPIP()`, `WiFi.scanNetworks()`, `WiFi.BSSIDstr()`, etc. all unchanged.
- **esp_wifi promiscuous** — `esp_wifi_set_promiscuous()`, `esp_wifi_set_channel()`, `esp_wifi_set_promiscuous_filter()` unchanged.
- **esp_http_server** — All `httpd_*` request/response functions (`httpd_resp_send`, `httpd_register_uri_handler`, etc.) unchanged.
- **FreeRTOS** — `xSemaphoreCreateMutex`, `xSemaphoreTake/Give`, `xQueueCreate/Send/Receive`, `pdMS_TO_TICKS` all unchanged. (20+ call sites across skyspy, wardriver, flockyou, skyspy_routes, flockyou_routes.)
- **Preferences/NVS** — `begin()`, `getBool()`, `putBool()`, `getString()`, `putString()`, etc. unchanged. (50+ calls in `nvs_store.cpp`.)
- **LittleFS** — `LittleFS.begin()`, `.open()`, `.exists()`, `.remove()` unchanged. Now a native ESP-IDF component.
- **mDNS** — `MDNS.begin()`, `MDNS.addService()` unchanged.
- **ESP methods** — `ESP.getFreeHeap()`, `ESP.getFreePsram()`, `ESP.restart()` unchanged.
- **Serial** — `Serial.begin()`, `Serial.println()`, `Serial.printf()` unchanged.
- **Arduino core** — `millis()`, `delay()`, `digitalRead()`, `pinMode()` unchanged.

---

## Memory Impact

No official benchmarks exist for ESP-IDF 4.4 vs 5.x on ESP32-S3. Community reports suggest:

| Metric | Expected Change | Notes |
|--------|----------------|-------|
| Flash size | +10-20% | New driver abstractions, mbedTLS 3.x (vs 2.x), esp_netif layer |
| Static DRAM (.bss/.data) | +5-15 KB | New components, wider structs |
| Free heap at runtime | −5-15 KB | From ~144 KB → ~129-139 KB estimated |
| PSRAM | Unchanged | 8 MB available, not constrained |

Current budget: 30.2% RAM, 20.4% Flash — plenty of headroom for the expected increase.

**Recommendation:** After migration, compare `ESP.getFreeHeap()` and `pio run` size output against current baseline.

---

## Risk Assessment

### pioarduino Community Fork

| Risk | Severity | Mitigation |
|------|----------|------------|
| No official PlatformIO support | Medium | pioarduino is actively maintained; large user base; Espressif employees contribute |
| PlatformIO Core updates could break fork | Low | Pin platform to specific release URL; fork tracks PIO internals closely |
| Library compatibility | Low | NimBLE 2.x, ArduinoJson 7.x, NeoPixel, TinyGPSPlus all support v3.x |
| Build system differences (sdkconfig) | Low | pioarduino handles sdkconfig through board definitions |

### Migration Effort

| Area | Files Affected | Estimated Effort |
|------|---------------|------------------|
| NimBLE 2.x migration | `ble_mgr.cpp/h` + 5 modules (7 files) | ~2 hours |
| DHCP server API | `wifi_mgr.cpp` (1 file, 4 lines) | ~15 min |
| HTTPS cert config | `server.cpp` (1 file, 4 lines) | ~15 min |
| Build flags | `platformio.ini` (1 file) | ~15 min |
| New `-Werror` warnings | Unknown | ~1-2 hours |
| Integration testing | Full device test | ~2-3 hours |
| **Total** | | **~6-8 hours** |

---

## Decision

**Proceed with the upgrade**, using the pioarduino community fork.

Rationale:

1. ESP-IDF 4.4 is end-of-life — no new security patches or bug fixes
2. NimBLE-Arduino 1.x is effectively abandoned; 2.x is the active line
3. The `cacert_pem` workaround is a maintenance trap waiting to confuse future contributors
4. Flash and RAM headroom is sufficient to absorb the expected increase
5. The migration is mechanical — no architectural changes, just API renames
6. pioarduino is the established community solution with broad adoption

## Implementation Plan

### Phase 1: Platform & Build (branch: `upgrade/esp-idf-5x`)

1. Update `platformio.ini`:

   ```ini
   platform = https://github.com/pioarduino/platform-espressif32/releases/download/55.03.37/platform-espressif32.zip
   lib_deps = h2zero/NimBLE-Arduino@^2.3.0  ; was ^1.4.0
   ```

2. Remove `-mfix-esp32-psram-cache-issue` from build flags
3. Run `pio run` — catalog all compilation errors

### Phase 2: NimBLE 2.x Migration

1. Update `src/hal/ble_mgr.h`: change `BLEListener::onBLEAdvertisement` to const pointer
2. Update `src/hal/ble_mgr.cpp`: rename callback class, fix `setPower()`, fix `start()` duration, fix `setScanCallbacks()`
3. Update all 5 modules' `onBLEAdvertisement()` signatures

### Phase 3: ESP-IDF 5.x API Changes

1. Update `src/hal/wifi_mgr.cpp`: replace `dhcps_*` with `esp_netif_dhcps_option()`
2. Update `src/web/server.cpp`: rename `cacert_pem` → `servercert`, remove workaround comment

### Phase 4: Build Clean & Verify

1. Fix any new `-Werror` warnings from ESP-IDF 5.x headers
2. Run `pio run` — confirm clean build
3. Run `pio test -e native` — confirm all 57 logic tests pass
4. Run web build chain — confirm all 18 web tests pass

### Phase 5: Device Testing

1. Flash to device, verify WiFi AP connectivity
2. Verify BLE scanning (70% duty cycle coexistence)
3. Verify HTTPS dashboard loads with valid cert
4. Verify all 5 modules function (enable/disable, routes, data)
5. Compare heap/flash usage against pre-upgrade baseline
6. Test scan mode (WiFi promiscuous + aggressive BLE)

## References

- [Arduino-ESP32 2.x to 3.0 Migration Guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/migration_guides/2.x_to_3.0.html)
- [NimBLE-Arduino 1.x to 2.x Migration Guide](https://h2zero.github.io/NimBLE-Arduino/md_1_8x__to2_8x__migration__guide.html)
- [ESP-IDF 5.0 Migration Guides](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/migration-guides/release-5.x/5.0/)
- [pioarduino/platform-espressif32](https://github.com/pioarduino/platform-espressif32)
- [PlatformIO Arduino-ESP32 v3 Support Status (Issue #1225)](https://github.com/platformio/platform-espressif32/issues/1225)
