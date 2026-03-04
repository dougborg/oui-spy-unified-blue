#include "ble_mgr.h"
#include <vector>

namespace hal {

static NimBLEScan* _scan = nullptr;
static std::vector<BLEListener*> _listeners;
static bool _aggressive = false;
static unsigned long _lastScanStart = 0;
static bool _initialized = false;

// Scan timing — must leave airtime for WiFi AP coexistence
static const int SCAN_DURATION = 3000;    // milliseconds per scan
static const int SCAN_RESTART_MS = 4000;  // gap between scans

// Dispatcher callback
class DispatchCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* device) override {
        for (auto* listener : _listeners) {
            listener->onBLEAdvertisement(device);
        }
    }
};

static DispatchCallbacks _callbacks;

void bleInit() {
    if (_initialized)
        return;
    NimBLEDevice::init("");
    NimBLEDevice::setPower(9); // Max TX power
    _scan = NimBLEDevice::getScan();
    _scan->setScanCallbacks(&_callbacks, false);
    _scan->setActiveScan(false); // Passive — saves radio time for WiFi coexistence
    _scan->setDuplicateFilter(false); // See every advertisement

    // Default scan params — ~70% BLE duty cycle
    // Units are 0.625ms: interval=160 → 100ms, window=112 → 70ms
    _scan->setInterval(160);
    _scan->setWindow(112);

    _initialized = true;
    Serial.println("[HAL] BLE manager initialized");
}

void bleAddListener(BLEListener* listener) {
    for (auto* l : _listeners) {
        if (l == listener)
            return; // Already registered
    }
    _listeners.push_back(listener);
}

void bleRemoveListener(BLEListener* listener) {
    _listeners.erase(std::remove(_listeners.begin(), _listeners.end(), listener), _listeners.end());
}

void bleRequestAggressiveScan(bool aggressive) {
    if (aggressive == _aggressive)
        return;
    _aggressive = aggressive;
    if (_scan) {
        if (_aggressive) {
            // Scan-only mode (no WiFi AP) — max duty cycle is safe
            _scan->setActiveScan(true);
            _scan->setInterval(80);
            _scan->setWindow(72);
            Serial.println("[HAL] BLE scan: aggressive (50ms/45ms, active)");
        } else {
            // Normal mode (WiFi AP active) — ~70% duty cycle
            _scan->setActiveScan(false);
            _scan->setInterval(160);
            _scan->setWindow(112);
            Serial.println("[HAL] BLE scan: normal (100ms/70ms, passive)");
        }
    }
}

NimBLEScan* bleGetScan() {
    return _scan;
}

void bleUpdate() {
    if (!_initialized || !_scan)
        return;
    if (_listeners.empty())
        return;

    unsigned long now = millis();

    // Restart scan if not scanning and interval elapsed
    if (!_scan->isScanning() && (now - _lastScanStart >= SCAN_RESTART_MS)) {
        _scan->start(SCAN_DURATION, false);
        _lastScanStart = now;
    }
}

} // namespace hal
