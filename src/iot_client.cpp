// iot_client.cpp
#include "iot_client.h"

IoTClient::IoTClient() 
    : _lastTelemetrySend(0), _initialized(false) {
}

bool IoTClient::begin() {
    // Load device token from storage
    if (!_storage.begin()) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Failed to initialize storage for IoT client");
        #endif
        return false;
    }
    
    _deviceToken = _storage.loadDeviceToken();
    
    if (_deviceToken.isEmpty()) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("No device token found. Please complete setup first.");
        #endif
        return false;
    }
    
    // Initialize the selected protocol
    bool success = false;
    
    #if COMMUNICATION_PROTOCOL == PROTOCOL_MQTT
        success = _protocol.begin(IOT_MQTT_BROKER, IOT_MQTT_PORT, _deviceToken);
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Initializing MQTT protocol...");
        #endif
    #elif COMMUNICATION_PROTOCOL == PROTOCOL_WEBSOCKET
        success = _protocol.begin(IOT_WEBSOCKET_URL, 443, "/ws", _deviceToken);
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Initializing WebSocket protocol...");
        #endif
    #elif COMMUNICATION_PROTOCOL == PROTOCOL_RESTAPI
        success = _protocol.begin(IOT_SERVER_URL, _deviceToken);
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Initializing REST API protocol...");
        #endif
    #endif
    
    if (success) {
        _initialized = true;
        #if ENABLE_SERIAL_DEBUG
        Serial.print("IoT client initialized with protocol: ");
        Serial.println(getProtocolName());
        #endif
    } else {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Failed to initialize IoT protocol");
        #endif
    }
    
    return success;
}

void IoTClient::loop() {
    if (!_initialized) return;
    
    _protocol.loop();
}

bool IoTClient::isConnected() {
    if (!_initialized) return false;
    
    #if COMMUNICATION_PROTOCOL == PROTOCOL_MQTT || COMMUNICATION_PROTOCOL == PROTOCOL_WEBSOCKET
        return _protocol.isConnected();
    #elif COMMUNICATION_PROTOCOL == PROTOCOL_RESTAPI
        // For REST API, we check connectivity less frequently
        static unsigned long lastCheck = 0;
        static bool lastStatus = false;
        
        if (millis() - lastCheck > 60000) { // Check every minute
            lastCheck = millis();
            lastStatus = _protocol.isConnected();
        }
        return lastStatus;
    #else
        return false;
    #endif
}

bool IoTClient::sendTelemetry(const TelemetryData& data) {
    if (!_initialized) return false;
    
    // Rate limiting for telemetry
    if (millis() - _lastTelemetrySend < TELEMETRY_SEND_INTERVAL_MS) {
        return false; // Too soon
    }
    
    bool success = false;
    
    #if COMMUNICATION_PROTOCOL == PROTOCOL_MQTT
        success = _protocol.publishTelemetry(data);
    #elif COMMUNICATION_PROTOCOL == PROTOCOL_WEBSOCKET
        success = _protocol.sendTelemetry(data);
    #elif COMMUNICATION_PROTOCOL == PROTOCOL_RESTAPI
        success = _protocol.sendTelemetry(data);
    #endif
    
    if (success) {
        _lastTelemetrySend = millis();
    }
    
    return success;
}

bool IoTClient::sendStatus(const String& status) {
    if (!_initialized) return false;
    
    #if COMMUNICATION_PROTOCOL == PROTOCOL_MQTT
        return _protocol.publishStatus(status);
    #elif COMMUNICATION_PROTOCOL == PROTOCOL_WEBSOCKET
        return _protocol.sendStatus(status);
    #elif COMMUNICATION_PROTOCOL == PROTOCOL_RESTAPI
        return _protocol.sendStatus(status);
    #else
        return false;
    #endif
}

bool IoTClient::sendConfig(const TankConfig& config) {
    if (!_initialized) return false;
    
    #if COMMUNICATION_PROTOCOL == PROTOCOL_MQTT
        return _protocol.publishConfig(config);
    #elif COMMUNICATION_PROTOCOL == PROTOCOL_WEBSOCKET
        return _protocol.sendConfig(config);
    #elif COMMUNICATION_PROTOCOL == PROTOCOL_RESTAPI
        return _protocol.sendConfig(config);
    #else
        return false;
    #endif
}

bool IoTClient::requestConfig() {
    if (!_initialized) return false;
    
    #if COMMUNICATION_PROTOCOL == PROTOCOL_MQTT
        return _protocol.requestConfig();
    #elif COMMUNICATION_PROTOCOL == PROTOCOL_WEBSOCKET
        return _protocol.requestConfig();
    #elif COMMUNICATION_PROTOCOL == PROTOCOL_RESTAPI
        String configJson;
        return _protocol.getConfig(configJson);
    #else
        return false;
    #endif
}

void IoTClient::setCommandCallback(void (*callback)(const CommandData& cmd)) {
    if (!_initialized) return;
    _protocol.setCommandCallback(callback);
}

void IoTClient::setConfigCallback(void (*callback)(const String& configJson)) {
    if (!_initialized) return;
    _protocol.setConfigCallback(callback);
}

String IoTClient::getProtocolName() {
    #if COMMUNICATION_PROTOCOL == PROTOCOL_MQTT
        return "MQTT";
    #elif COMMUNICATION_PROTOCOL == PROTOCOL_WEBSOCKET
        return "WebSocket";
    #elif COMMUNICATION_PROTOCOL == PROTOCOL_RESTAPI
        return "REST API";
    #else
        return "Unknown";
    #endif
}