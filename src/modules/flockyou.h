#pragma once

#include "module.h"
#include "../hal/ble_mgr.h"

class FlockyouModule : public IDetectionModule, public hal::BLEListener {
public:
    const char* name() override { return "flockyou"; }
    void setup() override;
    void loop() override;
    void onBLEAdvertisement(NimBLEAdvertisedDevice* device) override;
    void registerRoutes(AsyncWebServer& server) override;
    bool isEnabled() override;
    void setEnabled(bool enabled) override;
private:
    bool _enabled = true;
};
