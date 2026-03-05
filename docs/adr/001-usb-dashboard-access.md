# ADR-001: USB Dashboard Access

**Status:** Proposed
**Date:** 2026-03-04
**Decision makers:** colonelpanichacks

## Problem Statement

The OUI-SPY web dashboard is served over a WiFi AP (192.168.4.1). In scan mode the AP is disabled for WiFi promiscuous capture, making the dashboard inaccessible. A USB-based fallback would allow dashboard access during scan mode and provide an alternative when WiFi is unavailable.

Two network-over-USB approaches were evaluated: PPP over Serial and CDC-ECM (USB Ethernet). A third lightweight alternative emerged during analysis.

## Current Resource Budget

| State | Free Heap | Notes |
|-------|-----------|-------|
| Normal mode (WiFi AP + BLE 70% + all modules) | ~144 KB | Tested, stable at 70% BLE duty cycle |
| BLE disabled | ~195 KB | BLE stack consumes ~51 KB |
| ESP32-S3 total DRAM | 320 KB | Plus 8 MB PSRAM available |

Platform: Seeed XIAO ESP32-S3, `espressif32@^6.3.0`, Arduino framework.

---

## Option A: PPP over Serial

PPPoS (PPP over Serial) creates a real IP network interface over the USB-CDC serial port. The host runs `pppd` and gets a routed IP connection to the ESP32. The existing AsyncWebServer serves transparently on the new interface via lwIP.

### A: Memory Impact

- PPP stack overhead: ~20-40 KB heap
- Remaining free heap: ~104-124 KB (from 144 KB baseline)
- Flash: ~15-20 KB additional firmware

### A: Advantages

- Transparent to existing web server (lwIP auto-routes to all interfaces)
- Mature, well-understood protocol
- Moderate memory overhead

### A: Blockers

1. **Arduino framework does not expose PPP APIs.** ESP-IDF's lwIP supports PPPoS natively, but the Arduino-ESP32 wrapper does not surface these APIs. Enabling PPP requires either migrating to native ESP-IDF or building a custom Arduino-ESP32 fork. Both are major architectural changes. (Ref: [espressif/arduino-esp32#7203](https://github.com/espressif/arduino-esp32/issues/7203))

2. **Serial port contention.** The single USB-CDC port currently carries debug logging (`Serial.println`). PPP needs a dedicated byte stream. Options:
   - *Dual CDC ports:* ESP32-S3 has 6 USB endpoints, supporting max 2 CDC channels. However, COM port enumeration is unreliable across OSes — ports swap on reconnect.
   - *Multiplexed serial:* Run debug + PPP on one port with a framing protocol. Adds complexity to both firmware and host tooling.

3. **Host-side setup required.** Users must configure `pppd` with correct options. Not plug-and-play.

### Verdict: Not feasible without framework migration

---

## Option B: CDC-ECM / CDC-NCM (USB Ethernet)

The ESP32-S3 presents itself as a USB Ethernet adapter via TinyUSB. The host OS detects a new network interface automatically. The web server is accessible on both WiFi and USB interfaces.

### B: Memory Impact

- TinyUSB core + CDC-ECM class: ~10-30 KB heap
- Network buffers (no DMA, CPU polling): ~20-40 KB heap
- lwIP USB netif driver: ~20-30 KB heap
- **Total: ~50-100 KB overhead**
- Remaining free heap: ~44-94 KB (dangerously low for active web connections)

### B: Advantages

- Most seamless UX — plug USB in, browse to an IP
- No host-side software beyond a web browser

### B: Blockers

1. **macOS does not support CDC-ECM natively.** macOS requires CDC-NCM (Network Control Model). CDC-ECM devices are not recognized without third-party drivers. Since the primary development environment is macOS, this is a showstopper for the default configuration. (Ref: Apple `com.apple.driver.usb.cdc.ncm` only handles NCM class)

2. **Conflicts with `ARDUINO_USB_CDC_ON_BOOT`.** The project uses `-DARDUINO_USB_CDC_ON_BOOT=1` for serial debug output. This Arduino hardware CDC initialization conflicts with TinyUSB's CDC stack. Running both simultaneously is not supported — enabling TinyUSB requires disabling CDC-on-boot and managing all USB interfaces through TinyUSB, which breaks `Serial.println()` debugging.

3. **Network driver is WIP.** TinyUSB's network device class has no DMA support (CPU polling only), an unstable API, and no production ESP32 examples.

4. **Memory pressure.** With 50-100 KB overhead, remaining heap (44-94 KB) is insufficient to reliably handle concurrent web requests, JSON serialization, and module operations.

5. **Cross-platform fragmentation.** Windows needs RNDIS (separate USB class from ECM/NCM). Supporting all platforms requires implementing multiple USB network classes.

### Verdict: Not recommended

---

## Option C: Host-Side Serial Proxy (Recommended)

A lightweight approach that avoids new network stacks entirely. A Python script on the host bridges `localhost:8080` to the ESP32 over serial using a simple binary framing protocol. The firmware adds a minimal serial command handler (~2-3 KB) that dispatches requests to the existing route handlers.

### Architecture

```text
Browser ──HTTP──▶ Python proxy (localhost:8080) ──serial frames──▶ ESP32 handler
                                                                     │
                                                              Existing route
                                                              handler functions
                                                                     │
Browser ◀──HTTP── Python proxy ◀──serial frames────────────────── Response
```

### C: Memory Impact

- Firmware: ~2-3 KB flash for framing protocol + request dispatcher
- Heap: negligible (reuses existing route handler functions, no new buffers)
- Remaining free heap: ~144 KB (unchanged from current baseline)

### Firmware Component (`src/hal/serial_http.h/.cpp`)

- Watch for magic byte prefix on Serial (e.g., `0xAA 0x55`) to distinguish from debug text
- Read length-prefixed request: method (1 byte) + path length + path + body length + body
- Dispatch to existing route handler functions (same code paths as WiFi requests)
- Write back length-prefixed response: status code + content-type + body
- Non-framed serial bytes pass through as normal debug output

### Host Component (`tools/serial_dashboard.py`)

- `python3 tools/serial_dashboard.py [--port /dev/cu.usbmodem2101] [--baud 115200]`
- Starts HTTP server on `localhost:8080`
- Serves the dashboard static files directly from `web/dist/` (avoids serial bandwidth bottleneck for HTML/JS/CSS)
- Proxies `/api/*` requests over serial to the ESP32
- Prints non-framed serial data to stdout (preserves debug logging)
- Auto-detects serial port if only one USB-serial device is connected

### Bandwidth Consideration

Serial at 115200 baud = ~14 KB/s throughput. The dashboard SPA is ~150 KB gzipped — too slow to serve over serial. The proxy script serves static assets from the local `web/dist/` directory and only forwards API calls (`/api/*`) over serial. API payloads are small JSON (typically < 1 KB), well within serial bandwidth.

### C: Advantages

- Zero impact on existing memory budget
- Works on all platforms (Python 3 + pyserial)
- Works during scan mode (no WiFi AP needed)
- Serial debug output preserved
- No new libraries or framework changes in firmware
- Ships as a simple repo tool (`tools/serial_dashboard.py`)
- Can be extended to support WebSocket-style push notifications later

### Limitations

- Requires Python 3 + pyserial on the host
- API-only over serial; static assets served locally (not a problem in practice)
- Single-client (one serial port = one proxy instance)
- Higher latency than direct WiFi (~50-100ms per API call vs ~5-10ms over WiFi)

---

## Decision

**Adopt Option C (Host-Side Serial Proxy)** for USB dashboard access.

Options A and B are blocked by framework limitations (PPP not exposed in Arduino) and platform incompatibilities (CDC-ECM not supported on macOS) respectively, with both carrying significant memory overhead. Option C delivers the same user-facing capability — full dashboard access over USB — with negligible firmware cost and no architectural risk.

## Implementation Plan

1. Add `src/hal/serial_http.h/.cpp` — binary framing protocol + request dispatcher
2. Add `tools/serial_dashboard.py` — HTTP-to-serial bridge with static file serving
3. Add `tools/requirements.txt` — pyserial dependency
4. Update OpenAPI schema if new serial-specific endpoints are needed
5. Test: scan mode (no WiFi) with serial proxy serving dashboard

## References

- [espressif/arduino-esp32#7203 — PPP client support request](https://github.com/espressif/arduino-esp32/issues/7203)
- [ESP-IDF lwIP PPP documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/lwip.html)
- [TinyUSB CDC-ECM/RNDIS discussion](https://github.com/hathach/tinyusb/discussions/742)
- [ESP32-S3 USB CDC documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/usb_cdc.html)
