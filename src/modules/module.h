#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

// ============================================================================
// Module Interface — core lifecycle contract for all detection modules
// ============================================================================

class IModule {
  public:
    virtual ~IModule() = default;

    virtual const char* name() = 0;
    virtual void setup() = 0;
    virtual void loop() = 0;
    virtual bool isEnabled() = 0;
    virtual void setEnabled(bool enabled) = 0;

    // Route registration (modules delegate to free functions in src/web/)
    virtual void registerRoutes(AsyncWebServer& server) = 0;
};
