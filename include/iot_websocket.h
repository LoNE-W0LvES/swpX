// iot_websocket.h
#ifndef IOT_WEBSOCKET_H
#define IOT_WEBSOCKET_H

#include <WiFi.h>
#include <WebSocketsClient.h>
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

class IoTWebSocket {
public:
    IoTWebSocket();
    bool begin(const String& url, int port, const String& path, const String& deviceToken);
    void loop();
    
    // Connection management
    bool connect();
    bool isConnected();
    void disconnect();
    
    // Send telemetry
    bool sendTelemetry(const TelemetryData& data);
    
    // Send status
    bool sendStatus(const String& status);
    
    // Send config (for syncing)
    bool sendConfig(const TankConfig& config);
    
    // Request config from server
    bool requestConfig();
    
    // Set callback for received commands
    void setCommandCallback(void (*callback)(const CommandData& cmd));
    
    // Set callback for received config
    void setConfigCallback(void (*callback)(const String& configJson));
    
private:
    WebSocketsClient _wsClient;
    String _deviceToken;
    bool _isConnected;
    unsigned long _lastReconnectAttempt;
    unsigned long _lastPingTime;
    
    void (*_commandCallback)(const CommandData& cmd);
    void (*_configCallback)(const String& configJson);
    
    static void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    static IoTWebSocket* _instance;
    
    void handleEvent(WStype_t type, uint8_t* payload, size_t length);
    void handleMessage(const String& message);
    bool sendMessage(const String& type, const String& data);
};

#endif // IOT_WEBSOCKET_H

