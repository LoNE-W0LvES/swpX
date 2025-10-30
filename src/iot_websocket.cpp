// iot_websocket.cpp
#include "iot_websocket.h"
#include "config.h"

IoTWebSocket* IoTWebSocket::_instance = nullptr;

IoTWebSocket::IoTWebSocket() 
    : _isConnected(false),
      _commandCallback(nullptr),
      _configCallback(nullptr),
      _lastReconnectAttempt(0),
      _lastPingTime(0) {
    _instance = this;
}

bool IoTWebSocket::begin(const String& url, int port, const String& path, const String& deviceToken) {
    _deviceToken = deviceToken;
    
    // Parse URL to get host
    String host = url;
    host.replace("ws://", "");
    host.replace("wss://", "");
    
    // Setup WebSocket with authentication header
    _wsClient.beginSSL(host.c_str(), port, path.c_str());
    
    // Add device token to headers for authentication
    String authHeader = "Authorization: Bearer " + _deviceToken;
    _wsClient.setExtraHeaders(authHeader.c_str());
    
    _wsClient.onEvent(webSocketEvent);
    _wsClient.setReconnectInterval(5000);
    _wsClient.enableHeartbeat(15000, 3000, 2); // Ping every 15s, timeout 3s, 2 retries
    
    return true;
}

void IoTWebSocket::loop() {
    _wsClient.loop();
    
    // Send periodic ping to keep connection alive
    if (_isConnected && millis() - _lastPingTime > 30000) {
        _wsClient.sendPing();
        _lastPingTime = millis();
    }
}

bool IoTWebSocket::connect() {
    // Connection is handled automatically by the library
    return true;
}

bool IoTWebSocket::isConnected() {
    return _isConnected;
}

void IoTWebSocket::disconnect() {
    sendStatus("offline");
    _wsClient.disconnect();
    _isConnected = false;
}

bool IoTWebSocket::sendTelemetry(const TelemetryData& data) {
    if (!_isConnected) return false;
    
    JsonDocument doc;
    doc["type"] = "telemetry";
    
    JsonObject payload = doc.createNestedObject("payload");
    payload["timestamp"] = data.timestamp;
    payload["motorState"] = data.motorState;
    payload["waterLevel"] = data.waterLevel;
    payload["currentInflow"] = data.currentInflow;
    payload["maxInflow"] = data.maxInflow;
    payload["dailyUsage"] = data.dailyUsage;
    payload["monthlyUsage"] = data.monthlyUsage;
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    
    return _wsClient.sendTXT(jsonStr);
}

bool IoTWebSocket::sendStatus(const String& status) {
    if (!_isConnected) return false;
    
    JsonDocument doc;
    doc["type"] = "status";
    
    JsonObject payload = doc.createNestedObject("payload");
    payload["status"] = status;
    payload["timestamp"] = millis();
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    
    return _wsClient.sendTXT(jsonStr);
}

bool IoTWebSocket::sendConfig(const TankConfig& config) {
    if (!_isConnected) return false;
    
    JsonDocument doc;
    doc["type"] = "config";
    
    JsonObject payload = doc.createNestedObject("payload");
    payload["tankHeight"] = config.tankHeight;
    payload["tankLength"] = config.tankLength;
    payload["tankWidth"] = config.tankWidth;
    payload["tankRadius"] = config.tankRadius;
    payload["shape"] = (config.shape == RECTANGULAR) ? "rectangular" : "cylindrical";
    payload["upperThreshold"] = config.upperThreshold;
    payload["lowerThreshold"] = config.lowerThreshold;
    payload["maxInflow"] = config.maxInflow;
    payload["configVersion"] = config.configVersion;
    payload["lastModifiedSource"] = config.lastModifiedSource;
    payload["syncMode"] = (config.syncMode == DEVICE_PRIORITY) ? "device" : "cloud";
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    
    return _wsClient.sendTXT(jsonStr);
}

bool IoTWebSocket::requestConfig() {
    if (!_isConnected) return false;
    
    JsonDocument doc;
    doc["type"] = "request_config";
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    
    return _wsClient.sendTXT(jsonStr);
}

void IoTWebSocket::setCommandCallback(void (*callback)(const CommandData& cmd)) {
    _commandCallback = callback;
}

void IoTWebSocket::setConfigCallback(void (*callback)(const String& configJson)) {
    _configCallback = callback;
}

void IoTWebSocket::webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    if (_instance) {
        _instance->handleEvent(type, payload, length);
    }
}

void IoTWebSocket::handleEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            #if ENABLE_SERIAL_DEBUG
            Serial.println("WebSocket disconnected");
            #endif
            _isConnected = false;
            break;
            
        case WStype_CONNECTED:
            #if ENABLE_SERIAL_DEBUG
            Serial.println("WebSocket connected");
            #endif
            _isConnected = true;
            sendStatus("online");
            break;
            
        case WStype_TEXT:
            {
                String message = "";
                for (size_t i = 0; i < length; i++) {
                    message += (char)payload[i];
                }
                handleMessage(message);
            }
            break;
            
        case WStype_ERROR:
            #if ENABLE_SERIAL_DEBUG
            Serial.println("WebSocket error");
            #endif
            _isConnected = false;
            break;
            
        case WStype_PONG:
            #if ENABLE_SERIAL_DEBUG
            Serial.println("WebSocket pong received");
            #endif
            break;
            
        default:
            break;
    }
}

void IoTWebSocket::handleMessage(const String& message) {
    #if ENABLE_SERIAL_DEBUG
    Serial.print("WebSocket message: ");
    Serial.println(message);
    #endif
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        #if ENABLE_SERIAL_DEBUG
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
        #endif
        return;
    }
    
    String type = doc["type"].as<String>();
    
    if (type == "command" && _commandCallback) {
        CommandData cmd;
        cmd.command = doc["payload"]["command"].as<String>();
        
        JsonObject payloadObj = doc["payload"]["data"];
        String payloadStr;
        serializeJson(payloadObj, payloadStr);
        cmd.payload = payloadStr;
        
        _commandCallback(cmd);
    } else if (type == "config" && _configCallback) {
        String configStr;
        serializeJson(doc["payload"], configStr);
        _configCallback(configStr);
    }
}

bool IoTWebSocket::sendMessage(const String& type, const String& data) {
    if (!_isConnected) return false;
    
    JsonDocument doc;
    doc["type"] = type;
    doc["data"] = data;
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    
    return _wsClient.sendTXT(jsonStr);
}