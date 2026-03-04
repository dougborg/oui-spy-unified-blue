#pragma once

#include <Arduino.h>
#include <esp_http_server.h>

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

    // Route registration — modules register on both HTTPS and HTTP servers
    virtual void registerRoutes(httpd_handle_t https, httpd_handle_t http) = 0;
};
