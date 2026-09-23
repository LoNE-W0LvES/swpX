// iot_websocket.h
#ifndef IOT_WEBSOCKET_H
#define IOT_WEBSOCKET_H

#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "storage_manager.h"
#include "iot_types.h"

class IoTWebSocket {
public:
    IoTWebSocket();
    bool begin(const String& url, int port, const String& path, const String& deviceToken);
    void loop();
    
    bool connect();
    bool isConnected();
    void disconnect();
    
    bool sendTelemetry(const TelemetryData& data);
    bool sendStatus(const String& status);
    bool sendConfig(const TankConfig& config);
    bool requestConfig();
    
    void setCommandCallback(void (*callback)(const CommandData& cmd));
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
