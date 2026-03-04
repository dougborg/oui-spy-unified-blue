#pragma once

#include "../modules/module.h"
#include <esp_http_server.h>

// ============================================================================
// Unified Web Server — HTTPS:443 + HTTP:80 (captive portal)
// ============================================================================

namespace web {

void serverInit();                                      // Start HTTPS:443 + HTTP:80
httpd_handle_t getHTTPSServer();                        // For module route registration
httpd_handle_t getHTTPServer();                         // For module route registration
void registerSystemRoutes(IModule** modules, int count);
void serverBegin();                                     // No-op (servers started in init)

} // namespace web
