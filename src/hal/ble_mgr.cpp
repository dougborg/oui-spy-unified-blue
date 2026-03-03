#include "ble_mgr.h"
#include <vector>

namespace hal {

static NimBLEScan* _scan = nullptr;
static std::vector<BLEListener*> _listeners;
static bool _aggressive = false;
static unsigned long _lastScanStart = 0;
static bool _initialized = false;

// Scan duration in seconds
static const int SCAN_DURATION = 3;
static const int SCAN_RESTART_MS = 3000;

// Dispatcher callback
class DispatchCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* device) override {
        for (auto* listener : _listeners) {
            listener->onBLEAdvertisement(device);
        }
    }
};

static DispatchCallbacks _callbacks;

void bleInit() {
    if (_initialized) return;
    NimBLEDevice::init("");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // Max TX power
    _scan = NimBLEDevice::getScan();
    _scan->setAdvertisedDeviceCallbacks(&_callbacks, false);
    _scan->setActiveScan(true);
    _scan->setDuplicateFilter(false); // See every advertisement

    // Default scan params (relaxed)
    _scan->setInterval(100);
    _scan->setWindow(99);

    _initialized = true;
    Serial.println("[HAL] BLE manager initialized");
}

void bleAddListener(BLEListener* listener) {
    for (auto* l : _listeners) {
        if (l == listener) return; // Already registered
    }
    _listeners.push_back(listener);
}

void bleRemoveListener(BLEListener* listener) {
    _listeners.erase(
        std::remove(_listeners.begin(), _listeners.end(), listener),
        _listeners.end());
}

void bleRequestAggressiveScan(bool aggressive) {
    if (aggressive == _aggressive) return;
    _aggressive = aggressive;
    if (_scan) {
        if (_aggressive) {
            _scan->setInterval(16);
            _scan->setWindow(15);
            Serial.println("[HAL] BLE scan: aggressive (16ms/15ms)");
        } else {
            _scan->setInterval(100);
            _scan->setWindow(99);
            Serial.println("[HAL] BLE scan: normal (100ms/99ms)");
        }
    }
}

NimBLEScan* bleGetScan() { return _scan; }

void bleUpdate() {
    if (!_initialized || !_scan) return;
    if (_listeners.empty()) return;

    unsigned long now = millis();

    // Restart scan if not scanning and interval elapsed
    if (!_scan->isScanning() && (now - _lastScanStart >= SCAN_RESTART_MS)) {
        _scan->clearResults();
        _scan->start(SCAN_DURATION, nullptr, false);
        _lastScanStart = now;
    }
}

} // namespace hal
