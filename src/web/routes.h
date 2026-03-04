#pragma once

class AsyncWebServer;
class DetectorModule;
class FoxhunterModule;
class FlockyouModule;
class SkySpyModule;

void registerDetectorRoutes(AsyncWebServer& server, DetectorModule& mod);
void registerFoxhunterRoutes(AsyncWebServer& server, FoxhunterModule& mod);
void registerFlockyouRoutes(AsyncWebServer& server, FlockyouModule& mod);
void registerSkySpyRoutes(AsyncWebServer& server, SkySpyModule& mod);
