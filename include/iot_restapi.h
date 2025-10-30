// iot_restapi.h
#ifndef IOT_RESTAPI_H
#define IOT_RESTAPI_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "storage_manager.h"

struct TelemetryData {
    unsigned long timestamp;
    bool motorState;
    float waterLevel;
    float currentInflow;
    float maxInflow;
    float dailyUsage;
    float monthlyUsage;
};

struct CommandData {
    String command;
    String payload;
};

class IoTRestAPI {
public:
    IoTRestAPI();
    bool begin(const String& serverUrl, const String& deviceToken);
    void loop();
    
    // Connection check
    bool isConnected();
    
    // Send telemetry (POST)
    bool sendTelemetry(const TelemetryData& data);
    
    // Send status (POST)
    bool sendStatus(const String& status);
    
    // Send config (POST/PUT)
    bool sendConfig(const TankConfig& config);
    
    // Get config from server (GET)
    bool getConfig(String& configJson);
    
    // Check for pending commands (GET)
    bool checkCommands();
    
    // Set callback for received commands
    void setCommandCallback(void (*callback)(const CommandData& cmd));
    
    // Set callback for received config
    void setConfigCallback(void (*callback)(const String& configJson));
    
    // Manual polling (called periodically in loop)
    void pollServer();
    
private:
    HTTPClient _http;
    String _serverUrl;
    String _deviceToken;
    unsigned long _lastPollTime;
    unsigned long _pollInterval;
    
    void (*_commandCallback)(const CommandData& cmd);
    void (*_configCallback)(const String& configJson);
    
    bool sendRequest(const String& method, const String& endpoint, const String& payload, String& response);
    void addAuthHeader();
};

#endif // IOT_RESTAPI_H

