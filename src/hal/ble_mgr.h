#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>

// ============================================================================
// Unified BLE Scan Manager
// Single NimBLEScan dispatches advertisements to all registered listeners.
// ============================================================================

namespace hal {

// Callback interface for BLE advertisement listeners
class BLEListener {
public:
    virtual ~BLEListener() = default;
    virtual void onBLEAdvertisement(NimBLEAdvertisedDevice* device) = 0;
};

void bleInit();
void bleUpdate(); // Call every loop — restarts scan as needed

// Register/unregister listeners (modules)
void bleAddListener(BLEListener* listener);
void bleRemoveListener(BLEListener* listener);

// Request aggressive scan parameters (foxhunter needs 16ms/15ms)
// The manager uses the most aggressive params among all requestors.
void bleRequestAggressiveScan(bool aggressive);

// Get the scan object for advanced use
NimBLEScan* bleGetScan();

} // namespace hal
