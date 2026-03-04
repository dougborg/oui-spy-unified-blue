#pragma once

#include "../modules/module.h"
#include <ESPAsyncWebServer.h>

// ============================================================================
// Unified Web Server
// Single AsyncWebServer shared by all modules + system routes
// ============================================================================

namespace web {

void serverInit();
AsyncWebServer& getServer();

// Register system-level routes (/, /api/status, /api/modules, etc.)
void registerSystemRoutes(IModule** modules, int count);

// Start serving
void serverBegin();

} // namespace web
