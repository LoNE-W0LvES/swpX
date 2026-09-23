// iot_restapi.h
#ifndef IOT_RESTAPI_H
#define IOT_RESTAPI_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "storage_manager.h"
#include "iot_types.h"

class IoTRestAPI {
public:
    IoTRestAPI();
    bool begin(const String& serverUrl, const String& deviceToken);
    void loop();
    
    bool isConnected();
    bool sendTelemetry(const TelemetryData& data);
    bool sendStatus(const String& status);
    bool sendConfig(const TankConfig& config);
    bool getConfig(String& configJson);
    bool checkCommands();
    
    void setCommandCallback(void (*callback)(const CommandData& cmd));
    void setConfigCallback(void (*callback)(const String& configJson));
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
