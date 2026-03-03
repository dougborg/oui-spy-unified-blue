#pragma once

#include "../hal/ble_mgr.h"
#include "module.h"

class SkySpyModule : public IDetectionModule, public hal::BLEListener {
  public:
    const char* name() override {
        return "skyspy";
    }
    void setup() override;
    void loop() override;
    void onBLEAdvertisement(NimBLEAdvertisedDevice* device) override;
    void registerRoutes(AsyncWebServer& server) override;
    bool isEnabled() override;
    void setEnabled(bool enabled) override;

  private:
    bool _enabled = true;
};

// Static WiFi promiscuous callback (cannot be a member function)
void skyspyWiFiCallback(void* buf, wifi_promiscuous_pkt_type_t type);
