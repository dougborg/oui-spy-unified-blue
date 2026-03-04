# OUI SPY

Multi-mode surveillance detection and BLE/WiFi intelligence firmware for the **Seeed Studio XIAO ESP32-S3**.

One device. Four detection modules. All running simultaneously.

---

## Overview

OUI SPY runs all four detection modules at once — no boot menu, no mode switching, no rebooting. Connect to the WiFi AP, open the dashboard, and every module is active. Individual modules can be enabled/disabled from the web UI without restarting.

| Module | Detection Target | Method |
|--------|-----------------|--------|
| **Detector** | User-defined BLE devices | OUI prefix / full MAC watchlists |
| **Foxhunter** | Single BLE target | RSSI proximity tracking |
| **Flock-You** | Flock Safety / Raven cameras | BLE heuristics (MAC, name, mfg ID, UUIDs) |
| **Sky Spy** | FAA Remote ID drones | WiFi promiscuous + BLE (ODID/ASTM F3411) |

**WiFi AP:** `oui-spy` / `ouispy123` (configurable via web UI, persisted to NVS)
**Dashboard:** `http://192.168.4.1`

---

## Modules

### Detector

BLE alert tool that continuously scans for specific target devices by OUI prefix or full MAC address. When a match is found, the device triggers audible and visual alerts with cooldown logic to prevent alert fatigue.

- Configurable MAC/OUI watchlists via web interface
- NeoPixel + buzzer feedback on detection (new device: 3 beeps, re-detection: 2 beeps)
- Device alias support (name tracked devices)
- Cooldown system: 3s and 30s re-detection intervals with configurable cooldown periods

### Foxhunter

RSSI-based proximity tracker for hunting down a specific BLE device. Lock onto a target MAC address, then follow the signal strength. The buzzer cadence increases as you get closer — like a Geiger counter for Bluetooth.

- Select target by entering a MAC address (validated format)
- Audio feedback rate scales inversely with distance
- Web interface for target selection and live RSSI monitoring

### Flock-You

Detects Flock Safety surveillance cameras, Raven gunshot detectors, and related monitoring hardware using BLE-only heuristics. All detections are stored in memory and can be exported.

**Detection methods:**

- **MAC prefix matching** — 20 known Flock Safety OUI prefixes
- **BLE device name patterns** — case-insensitive substring matching for `FS Ext Battery`, `Penguin`, `Flock`, `Pigvision`
- **BLE manufacturer company ID** — `0x09C8` (XUNTONG), associated with Flock Safety hardware. *Sourced from [wgreenberg/flock-you](https://github.com/wgreenberg/flock-you).*
- **Raven service UUID matching** — identifies Raven gunshot detection units by BLE GATT service UUIDs
- **Raven firmware version estimation** — determines approximate firmware version (1.1.x / 1.2.x / 1.3.x) based on advertised UUIDs

**Features:**

- GPS wardriving via browser Geolocation API (phone GPS tags each detection with coordinates)
- JSON, CSV, and KML export of all detections
- Session persistence via LittleFS (previous session available after reboot)
- Thread-safe detection storage (up to 200 unique devices) with FreeRTOS mutex

**Enabling GPS (Android Chrome):**

Because the dashboard is served over HTTP, Chrome requires a one-time flag change for location access:

1. Open `chrome://flags` in a new tab
2. Search for **"Insecure origins treated as secure"**
3. Add `http://192.168.4.1` to the text field and set to **Enabled**
4. Tap **Relaunch**

> **Note:** iOS Safari does not support Geolocation over HTTP. GPS wardriving requires Android with Chrome.

### Sky Spy

Passive drone detection via FAA Remote ID (Open Drone ID) using both WiFi promiscuous mode and BLE advertisement scanning. Parses ASTM F3411 compliant broadcasts.

- WiFi beacon vendor IE parsing for ODID frames
- WiFi NAN action frame MAC extraction
- BLE ODID advertisement detection
- Captures drone serial numbers, operator/UAV IDs, location, altitude, speed, heading
- Tracks up to 8 simultaneous drones with 7s activity timeout

---

## Hardware

**Board:** Seeed Studio XIAO ESP32-S3

| Pin | Function |
|-----|----------|
| GPIO 3 | Piezo buzzer |
| GPIO 21 | NeoPixel LED |
| GPIO 0 | BOOT button (hold 1.5s to restart) |

**Flash layout:** Custom partition table with ~6MB app + ~2MB LittleFS data. See `partitions.csv`.

---

## Architecture

```text
src/
  app/            main.cpp — orchestrator, module lifecycle, boot sequence
  hal/            Hardware abstraction layer
    ble_mgr       BLE scan management + listener dispatch
    buzzer        Non-blocking sound effect state machine
    buzzer_logic  Pure testable buzzer math (proximity interval)
    gps           GPS service (hardware UART + phone browser API)
    neopixel      LED animation state machine
    neopixel_logic Pure testable LED math (breathing, flash)
    notify        Notification abstraction (semantic events → buzzer + LED)
    wifi_mgr      WiFi AP + promiscuous mode management
    led, pins, mac_util  Pin definitions and utilities
  modules/        Detection engines
    detector      BLE OUI/MAC watchlist scanner
    detector_logic Pure testable matching, cooldown, validation
    flockyou      Flock Safety / Raven BLE detector
    flockyou_logic Pure testable detection dedup, timing, name sanitization
    foxhunter     RSSI proximity tracker
    foxhunter_logic Pure testable proximity state machine, target matching
    skyspy        Open Drone ID detector (WiFi + BLE)
    skyspy_logic  Pure testable beacon IE parsing, slot allocation, ODID checks
    module.h      IModule interface (lifecycle contract)
  protocols/      Third-party protocol implementations
    opendroneid   ASTM F3411 Open Drone ID codec (C library)
    wifi          ODID WiFi frame builder/parser
    odid_wifi.h   ODID WiFi type definitions
  storage/        Persistence layer
    nvs_store     Typed NVS accessors for all 6 namespaces
  web/            Web server and API routes
    server        Core server, system routes (status, modules, AP, GPS, buzzer)
    dashboard.h   Embedded HTML SPA
    routes.h      Route registration declarations
    detector_routes, foxhunter_routes, flockyou_routes, skyspy_routes
```

### Key Design Patterns

- **IModule interface:** All modules implement `IModule` (name, setup, loop, isEnabled, setEnabled, registerRoutes). BLE callbacks come from a separate `hal::BLEListener` mixin.
- **Pure logic extraction:** Each module has a companion `*_logic.h/cpp` with pure functions (no Arduino dependencies) that can be compiled and tested natively.
- **Notification abstraction:** Modules call `hal::notify(NOTIFY_DET_NEW_DEVICE)` instead of directly coupling to buzzer/neopixel APIs. All audio/visual behavior is centralized in `hal/notify.cpp`.
- **Storage abstraction:** All NVS access goes through typed functions in `storage::` namespace. Six NVS namespaces are encapsulated behind a clean API.
- **Route separation:** Web route handlers live in dedicated `src/web/*_routes.cpp` files, keeping module code focused on detection logic.
- **ArduinoJson responses:** All JSON API responses use ArduinoJson v7 for proper string escaping and type safety.

### NVS Namespaces

| Namespace | Purpose | Keys |
|-----------|---------|------|
| `ouispy-mod` | Module enable/disable | `detector`, `foxhunter`, `flockyou`, `skyspy` |
| `ouispy-ap` | WiFi AP config | `ssid`, `pass` |
| `ouispy-bz` | Buzzer state | `on` |
| `ouispy` | Detector filters/aliases | `filterCount`, `id_N`, `mac_N`, `desc_N`, `buzzerEnabled`, `ledEnabled`, `aliasCount`, `alias_mac_N`, `alias_name_N` |
| `ouispy-dev` | Detector device history | `count`, `dm_N`, `dr_N`, `dl_N`, `df_N` |
| `tracker` | Foxhunter target | `targetMAC` |

---

## API Endpoints

### System

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/` | Dashboard HTML SPA |
| GET | `/api/status` | System status (uptime, heap, PSRAM, buzzer) |
| GET | `/api/modules` | Module list with enabled state |
| POST | `/api/modules/toggle` | Enable/disable a module |
| POST | `/api/buzzer` | Toggle buzzer on/off |
| GET | `/api/gps` | GPS status (lat, lon, accuracy, satellite count) |
| GET | `/api/ap` | Current AP SSID |
| POST | `/api/ap` | Set AP SSID/password (triggers reboot) |
| POST | `/api/reset` | Reboot device |

### Detector API

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/detector/filters` | Get MAC/OUI watchlist |
| POST | `/api/detector/filters` | Save filters + buzzer/LED settings |
| GET | `/api/detector/devices` | Get detected device list |
| POST | `/api/detector/alias` | Set device alias (max 32 chars) |
| POST | `/api/detector/clear-devices` | Clear device history |
| POST | `/api/detector/clear-filters` | Clear all filters |

### Foxhunter API

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/foxhunter/status` | Target info and current RSSI |
| POST | `/api/foxhunter/target` | Set target MAC (validated format) |
| GET | `/api/foxhunter/rssi` | Live RSSI polling |
| POST | `/api/foxhunter/clear` | Clear target |

### Flock-You API

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/flockyou/detections` | All current detections (JSON stream) |
| GET | `/api/flockyou/stats` | Detection stats (total, raven count, GPS info) |
| GET | `/api/flockyou/gps` | Submit phone GPS coordinates |
| GET | `/api/flockyou/patterns` | Detection pattern database |
| GET | `/api/flockyou/export/json` | Download detections as JSON |
| GET | `/api/flockyou/export/csv` | Download detections as CSV |
| GET | `/api/flockyou/export/kml` | Download detections as KML |
| GET | `/api/flockyou/history` | Previous session data |
| GET | `/api/flockyou/history/json` | Download previous session |
| GET | `/api/flockyou/clear` | Clear all detections |

### Sky Spy API

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/skyspy/drones` | All tracked drones with telemetry |
| GET | `/api/skyspy/status` | Active drone count and range status |

---

## Building and Flashing

### Quick Flash (no PlatformIO needed)

```bash
pip install esptool pyserial
python flash.py                        # auto-detect bin from firmware/ folder
python flash.py my_firmware.bin        # flash a specific file
python flash.py --erase                # full erase before flashing
```

### Building from Source

Requires [PlatformIO](https://platformio.org/).

```bash
pio run                     # build
pio run -t upload           # flash directly
pio device monitor          # serial output (115200 baud)
```

### Task Runner (just)

A `Justfile` is included for common commands.

```bash
# macOS
brew install just

# Ubuntu/Debian
sudo apt-get install -y just
```

Common tasks:

```bash
just build
just upload
just monitor
just flash
just flash firmware/my_firmware.bin
```

Quality tasks:

```bash
just setup-dev          # install dev dependencies (pre-commit, pytest, gcovr)
just test               # run Python tests
just test-cpp           # run native C++ tests
just coverage-cpp       # run tests + generate coverage report
just analyze-cpp        # run cppcheck static analysis
just lint               # run pre-commit (formatting, linting)
just web-lint           # run Biome linter on web source
just web-test           # run Vitest web tests
just quality            # run all quality checks
```

### Dev Container (VS Code)

This repo includes a ready-to-use devcontainer in `.devcontainer/` with PlatformIO, Node.js, pre-commit hook environments, and all dev tooling preinstalled.

1. Open the project in VS Code
2. Run **Dev Containers: Reopen in Container**
3. Build with `pio run`

The devcontainer supports multiple profiles. See `.devcontainer/README.md` for Linux USB passthrough and host-specific setup.

> **macOS note:** USB passthrough from containers can be limited. Build in-container (`pio run`) and flash from host (`python flash.py`).

---

## Testing and Quality

### Test Suites

| Suite | Environment | Tests | Coverage |
|-------|-------------|-------|----------|
| `test_native_smoke` | `native` | 45 tests | Logic modules at 75-100% |
| `test_native_parser_edge` | `native_parser` | 8 tests | ODID parser edge cases |
| `web/src/**/*.test.{ts,tsx}` | `jsdom` (Vitest) | 18 tests | API client, polling hook, UI components |

C++ tests run natively (no ESP32 needed) using Unity test framework. They exercise pure logic functions extracted from each module. Web tests run via Vitest with jsdom, using Testing Library for component tests.

**What's tested (firmware):**

- Detector: MAC validation, normalization, filter matching, cooldown state machine
- Foxhunter: proximity tick evaluation, target match events (first acquisition, reacquired, update, lost)
- Flockyou: detection matching helpers, firmware estimation, detection dedup, heartbeat timing, name sanitization
- Sky Spy: ODID vendor IE detection, beacon IE parsing, UAV slot allocation/eviction, BLE payload checks, NAN frame detection, active UAV counting
- Buzzer: proximity interval calculation
- NeoPixel: breathing and flash math
- ODID codec: encode/decode roundtrips, pack/unpack, NAN frame building
- WiFi: frame building, processing, truncation/corruption rejection

**What's tested (web dashboard):**

- API client: fetchJSON, postForm, postEmpty — success responses and error handling
- usePoll hook: mount fetch, interval polling, visibility pause/resume, cleanup
- Toggle component: rendering, state display, click handling
- StatCard component: value/label rendering, custom styling

### CI Pipeline

The GitHub Actions CI runs on every push to `master` and on PRs:

- **quality:** pre-commit (formatting, linting) + pytest
- **native-tests:** PlatformIO native tests + gcovr coverage report (35% line threshold)
- **web-build:** Biome lint + Vitest tests + Vite production build + 100KB bundle size gate
- **build-firmware:** PlatformIO firmware build with `-Werror` + 2MB binary size gate
- **static-analysis:** cppcheck on all maintained sources
- **devcontainer:** Docker image build, smoke test, and publish to GHCR (on devcontainer file changes)

### Code Quality

- `-Werror` enabled — all warnings are build errors
- Firmware binary size gate: CI fails if `.bin` exceeds 2MB
- `clang-format` v18.1.8 enforced via pre-commit
- `cppcheck` static analysis on all maintained source directories
- [Biome](https://biomejs.dev/) linter + formatter for web source (recommended rules, import organizer)
- [Vitest](https://vitest.dev/) + Testing Library for web component and hook tests
- ArduinoJson for all JSON API responses (proper escaping, no injection risk)
- Input validation on all POST endpoints (MAC format, string lengths, WPA2 password requirements)
- Bounds checking on WiFi promiscuous callbacks (beacon IE parsing, NAN frame handling)

---

## Dependency Management

| Source | Manages |
|--------|---------|
| `platformio.ini` | Firmware platform and libraries (`NimBLE-Arduino`, `ESP Async WebServer`, `ArduinoJson`, `Adafruit NeoPixel`, `TinyGPSPlus`) |
| `requirements-dev.txt` | Python dev tools (`pre-commit`, `pytest`, `gcovr`) |
| `web/package.json` | Web dashboard dependencies (Preact, Tailwind, Vitest, Biome, openapi-typescript) |
| `.devcontainer/Dockerfile` | Container toolchain (Python, Node.js, PlatformIO CLI, `just`, `clang-format`, `cppcheck`, pre-commit hook environments) |
| `.devcontainer/devcontainer.json` | VS Code extension dependencies |

Automation:

- **Dependabot** for Dockerfile base image, GitHub Actions, pip, and npm updates
- **PlatformIO Dependency Smoke** workflow (weekly) — installs packages and runs `pio run`
- **Devcontainer** workflow — builds, smoke-tests, and publishes the devcontainer image to GHCR
- **CI** workflow — PR/push validation
- **Release Firmware** workflow — tag-based GitHub Releases with firmware binaries and checksums
- **CodeQL** workflow — C/C++ security scanning

---

## Acknowledgments

**Will Greenberg** ([@wgreenberg](https://github.com/wgreenberg)) — His [flock-you](https://github.com/wgreenberg/flock-you) fork was instrumental in improving the Flock Safety detection heuristics. The BLE manufacturer company ID detection method (`0x09C8` XUNTONG) was sourced directly from his work. Thank you for the research and for making it open.

---

## OUI-SPY Firmware Ecosystem

| Firmware | Description | Board |
|----------|-------------|-------|
| **[OUI-SPY Unified](https://github.com/colonelpanichacks/oui-spy-unified-blue)** | Multi-mode BLE + WiFi detector (this project) | ESP32-S3 |
| **[OUI-SPY Detector](https://github.com/colonelpanichacks/ouispy-detector)** | Targeted BLE scanner with OUI filtering | ESP32-S3 |
| **[OUI-SPY Foxhunter](https://github.com/colonelpanichacks/ouispy-foxhunter)** | RSSI-based proximity tracker | ESP32-S3 |
| **[Flock You](https://github.com/colonelpanichacks/flock-you)** | Flock Safety / Raven surveillance detection | ESP32-S3 |
| **[Sky-Spy](https://github.com/colonelpanichacks/Sky-Spy)** | Drone Remote ID detection | ESP32-S3 / ESP32-C5 |
| **[Remote-ID-Spoofer](https://github.com/colonelpanichacks/Remote-ID-Spoofer)** | WiFi Remote ID spoofer & simulator with swarm mode | ESP32-S3 |
| **[OUI-SPY UniPwn](https://github.com/colonelpanichacks/Oui-Spy-UniPwn)** | Unitree robot exploitation system | ESP32-S3 |

---

## Author

colonelpanichacks

---

## Disclaimer

This tool is intended for security research, privacy auditing, and educational purposes. Detecting the presence of surveillance hardware in public spaces is legal in most jurisdictions. Always comply with local laws regarding wireless scanning and signal interception. The authors are not responsible for misuse.
