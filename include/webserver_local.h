// webserver_local.h
#ifndef WEBSERVER_LOCAL_H
#define WEBSERVER_LOCAL_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include "storage_manager.h"
#include "tank_calculator.h"
#include "pump_controller.h"
#include "water_tracker.h"

class WebServerLocal {
public:
    WebServerLocal();
    
    // Initialize web server (returns false if WiFi not available)
    bool begin(StorageManager* storage, TankCalculator* calculator, 
               PumpController* pump, WaterTracker* tracker);
    
    // Update with current system data
    void updateData(float waterLevel, float currentInflow, float maxInflow);
    
    // Check if server is running
    bool isRunning();
    
    // Stop server
    void stop();
    
private:
    AsyncWebServer* _server;
    StorageManager* _storage;
    TankCalculator* _calculator;
    PumpController* _pump;
    WaterTracker* _tracker;
    
    bool _isRunning;
    
    // Current data
    float _waterLevel;
    float _currentInflow;
    float _maxInflow;
    
    // Route handlers
    void setupRoutes();
    void handleRoot(AsyncWebServerRequest* request);
    void handleStatus(AsyncWebServerRequest* request);
    void handleTelemetry(AsyncWebServerRequest* request);
    void handlePumpOn(AsyncWebServerRequest* request);
    void handlePumpOff(AsyncWebServerRequest* request);
    void handleSetMode(AsyncWebServerRequest* request);
    void handleGetConfig(AsyncWebServerRequest* request);
    void handleSetConfig(AsyncWebServerRequest* request);
    void handleWiFiScan(AsyncWebServerRequest* request);
    void handleWiFiConnect(AsyncWebServerRequest* request);
    void handleSetup(AsyncWebServerRequest* request);
    void handleUsageStats(AsyncWebServerRequest* request);
    
    // Authentication
    bool checkAuth(AsyncWebServerRequest* request);
    
    // CORS headers
    void addCORSHeaders(AsyncWebServerResponse* response);
};

#endif // WEBSERVER_LOCAL_H

