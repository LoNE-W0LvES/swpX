// iot_mqtt.cpp
#include "iot_mqtt.h"
#include "config.h"

IoTMQTT* IoTMQTT::_instance = nullptr;

IoTMQTT::IoTMQTT() 
    : _mqttClient(_wifiClient), 
      _commandCallback(nullptr), 
      _configCallback(nullptr),
      _lastReconnectAttempt(0) {
    _instance = this;
}

bool IoTMQTT::begin(const String& broker, int port, const String& deviceToken) {
    _deviceToken = deviceToken;
    
    // Setup topics based on device token
    _topicTelemetry = "devices/" + _deviceToken + "/telemetry";
    _topicCommands = "devices/" + _deviceToken + "/commands";
    _topicConfig = "devices/" + _deviceToken + "/config";
    _topicStatus = "devices/" + _deviceToken + "/status";
    
    _mqttClient.setServer(broker.c_str(), port);
    _mqttClient.setCallback(mqttCallback);
    _mqttClient.setKeepAlive(60);
    _mqttClient.setSocketTimeout(10);
    
    return connect();
}

void IoTMQTT::loop() {
    if (!_mqttClient.connected()) {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt > 5000) {
            _lastReconnectAttempt = now;
            if (connect()) {
                _lastReconnectAttempt = 0;
            }
        }
    } else {
        _mqttClient.loop();
    }
}

bool IoTMQTT::connect() {
    if (_mqttClient.connected()) return true;
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Connecting to MQTT broker...");
    #endif
    
    // Use device token as client ID and username
    String clientId = "pump_" + _deviceToken;
    
    if (_mqttClient.connect(clientId.c_str(), _deviceToken.c_str(), "")) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("MQTT connected!");
        #endif
        
        // Subscribe to command and config topics
        _mqttClient.subscribe(_topicCommands.c_str());
        _mqttClient.subscribe(_topicConfig.c_str());
        
        // Publish online status
        publishStatus("online");
        
        return true;
    }
    
    #if ENABLE_SERIAL_DEBUG
    Serial.print("MQTT connection failed, rc=");
    Serial.println(_mqttClient.state());
    #endif
    
    return false;
}

bool IoTMQTT::isConnected() {
    return _mqttClient.connected();
}

void IoTMQTT::disconnect() {
    publishStatus("offline");
    _mqttClient.disconnect();
}

bool IoTMQTT::publishTelemetry(const TelemetryData& data) {
    if (!_mqttClient.connected()) return false;
    
    JsonDocument doc;
    doc["timestamp"] = data.timestamp;
    doc["motorState"] = data.motorState;
    doc["waterLevel"] = data.waterLevel;
    doc["currentInflow"] = data.currentInflow;
    doc["maxInflow"] = data.maxInflow;
    doc["dailyUsage"] = data.dailyUsage;
    doc["monthlyUsage"] = data.monthlyUsage;
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    
    return _mqttClient.publish(_topicTelemetry.c_str(), jsonStr.c_str(), false);
}

bool IoTMQTT::publishStatus(const String& status) {
    if (!_mqttClient.connected()) return false;
    
    JsonDocument doc;
    doc["status"] = status;
    doc["timestamp"] = millis();
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    
    return _mqttClient.publish(_topicStatus.c_str(), jsonStr.c_str(), true); // Retained
}

bool IoTMQTT::publishConfig(const TankConfig& config) {
    if (!_mqttClient.connected()) return false;
    
    JsonDocument doc;
    doc["tankHeight"] = config.tankHeight;
    doc["tankLength"] = config.tankLength;
    doc["tankWidth"] = config.tankWidth;
    doc["tankRadius"] = config.tankRadius;
    doc["shape"] = (config.shape == RECTANGULAR) ? "rectangular" : "cylindrical";
    doc["upperThreshold"] = config.upperThreshold;
    doc["lowerThreshold"] = config.lowerThreshold;
    doc["maxInflow"] = config.maxInflow;
    doc["configVersion"] = config.configVersion;
    doc["lastModifiedSource"] = config.lastModifiedSource;
    doc["syncMode"] = (config.syncMode == DEVICE_PRIORITY) ? "device" : "cloud";
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    
    return _mqttClient.publish(_topicConfig.c_str(), jsonStr.c_str(), true); // Retained
}

bool IoTMQTT::requestConfig() {
    if (!_mqttClient.connected()) return false;
    
    JsonDocument doc;
    doc["action"] = "get_config";
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    
    String requestTopic = "devices/" + _deviceToken + "/config/request";
    return _mqttClient.publish(requestTopic.c_str(), jsonStr.c_str());
}

void IoTMQTT::setCommandCallback(void (*callback)(const CommandData& cmd)) {
    _commandCallback = callback;
}

void IoTMQTT::setConfigCallback(void (*callback)(const String& configJson)) {
    _configCallback = callback;
}

void IoTMQTT::mqttCallback(char* topic, byte* payload, unsigned int length) {
    if (_instance) {
        String topicStr = String(topic);
        String payloadStr = "";
        for (unsigned int i = 0; i < length; i++) {
            payloadStr += (char)payload[i];
        }
        _instance->handleMessage(topicStr, payloadStr);
    }
}

void IoTMQTT::handleMessage(String topic, String payload) {
    #if ENABLE_SERIAL_DEBUG
    Serial.print("MQTT message on topic: ");
    Serial.println(topic);
    Serial.print("Payload: ");
    Serial.println(payload);
    #endif
    
    if (topic == _topicCommands && _commandCallback) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            CommandData cmd;
            cmd.command = doc["command"].as<String>();
            cmd.payload = doc["payload"].as<String>();
            _commandCallback(cmd);
        }
    } else if (topic == _topicConfig && _configCallback) {
        _configCallback(payload);
    }
}