#pragma once

#include <esp_http_server.h>

class DetectorModule;
class FoxhunterModule;
class FlockyouModule;
class SkySpyModule;
class WardriverModule;

void registerDetectorRoutes(httpd_handle_t https, httpd_handle_t http, DetectorModule& mod);
void registerFoxhunterRoutes(httpd_handle_t https, httpd_handle_t http, FoxhunterModule& mod);
void registerFlockyouRoutes(httpd_handle_t https, httpd_handle_t http, FlockyouModule& mod);
void registerSkySpyRoutes(httpd_handle_t https, httpd_handle_t http, SkySpyModule& mod);
void registerWardriverRoutes(httpd_handle_t https, httpd_handle_t http, WardriverModule& mod);
