// iot_mqtt.h
#ifndef IOT_MQTT_H
#define IOT_MQTT_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "storage_manager.h"
#include "iot_types.h"

class IoTMQTT {
public:
    IoTMQTT();
    bool begin(const String& broker, int port, const String& deviceToken);
    void loop();
    
    bool connect();
    bool isConnected();
    void disconnect();
    
    bool publishTelemetry(const TelemetryData& data);
    bool publishStatus(const String& status);
    bool publishConfig(const TankConfig& config);
    bool requestConfig();
    
    void setCommandCallback(void (*callback)(const CommandData& cmd));
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
    static IoTMQTT* _instance;
    
    void handleMessage(String topic, String payload);
    unsigned long _lastReconnectAttempt;
};

#endif // IOT_MQTT_H
