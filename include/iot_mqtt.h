// iot_mqtt.h
#ifndef IOT_MQTT_H
#define IOT_MQTT_H

#include <WiFi.h>
#include <PubSubClient.h>
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
    String command;  // "pump_on", "pump_off", "update_config", "ota_update", etc.
    String payload;  // JSON payload for additional data
};

class IoTMQTT {
public:
    IoTMQTT();
    bool begin(const String& broker, int port, const String& deviceToken);
    void loop();
    
    // Connection management
    bool connect();
    bool isConnected();
    void disconnect();
    
    // Publish telemetry
    bool publishTelemetry(const TelemetryData& data);
    
    // Publish status
    bool publishStatus(const String& status);
    
    // Publish config (for syncing)
    bool publishConfig(const TankConfig& config);
    
    // Request config from server
    bool requestConfig();
    
    // Set callback for received commands
    void setCommandCallback(void (*callback)(const CommandData& cmd));
    
    // Set callback for received config
    void setConfigCallback(void (*callback)(const String& configJson));
    
private:
    WiFiClient _wifiClient;
    PubSubClient _mqttClient;
    String _deviceToken;
    String _topicTelemetry;
    String _topicCommands;
    String _topicConfig;
    String _topicStatus;
    
    void (*_commandCallback)(const CommandData& cmd);
    void (*_configCallback)(const String& configJson);
    
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
    static IoTMQTT* _instance; // For static callback
    
    void handleMessage(String topic, String payload);
    unsigned long _lastReconnectAttempt;
};

#endif // IOT_MQTT_H

