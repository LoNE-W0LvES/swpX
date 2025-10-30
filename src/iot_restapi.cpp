// iot_restapi.cpp
#include "iot_restapi.h"
#include "config.h"

IoTRestAPI::IoTRestAPI() 
    : _commandCallback(nullptr),
      _configCallback(nullptr),
      _lastPollTime(0),
      _pollInterval(30000) { // Poll every 30 seconds
}

bool IoTRestAPI::begin(const String& serverUrl, const String& deviceToken) {
    _serverUrl = serverUrl;
    _deviceToken = deviceToken;
    
    // Test connection
    return isConnected();
}

void IoTRestAPI::loop() {
    // Automatically poll for commands and config updates
    if (millis() - _lastPollTime > _pollInterval) {
        _lastPollTime = millis();
        pollServer();
    }
}

bool IoTRestAPI::isConnected() {
    // Check if we can reach the server
    _http.begin(_serverUrl + "/api/v1/ping");
    addAuthHeader();
    
    int httpCode = _http.GET();
    _http.end();
    
    return (httpCode == HTTP_CODE_OK);
}

bool IoTRestAPI::sendTelemetry(const TelemetryData& data) {
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
    
    String response;
    return sendRequest("POST", "/api/v1/telemetry", jsonStr, response);
}

bool IoTRestAPI::sendStatus(const String& status) {
    JsonDocument doc;
    doc["status"] = status;
    doc["timestamp"] = millis();
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    
    String response;
    return sendRequest("POST", "/api/v1/status", jsonStr, response);
}

bool IoTRestAPI::sendConfig(const TankConfig& config) {
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
    
    String response;
    return sendRequest("PUT", "/api/v1/config", jsonStr, response);
}

bool IoTRestAPI::getConfig(String& configJson) {
    String response;
    if (sendRequest("GET", "/api/v1/config", "", response)) {
        configJson = response;
        return true;
    }
    return false;
}

bool IoTRestAPI::checkCommands() {
    String response;
    if (sendRequest("GET", "/api/v1/commands", "", response)) {
        if (response.length() > 0 && response != "[]") {
            // Parse commands array
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, response);
            
            if (!error && doc.is<JsonArray>()) {
                JsonArray commands = doc.as<JsonArray>();
                
                for (JsonObject cmdObj : commands) {
                    if (_commandCallback) {
                        CommandData cmd;
                        cmd.command = cmdObj["command"].as<String>();
                        
                        String payloadStr;
                        serializeJson(cmdObj["payload"], payloadStr);
                        cmd.payload = payloadStr;
                        
                        _commandCallback(cmd);
                    }
                    
                    // Acknowledge command
                    String commandId = cmdObj["id"].as<String>();
                    JsonDocument ackDoc;
                    ackDoc["commandId"] = commandId;
                    String ackStr;
                    serializeJson(ackDoc, ackStr);
                    
                    String ackResponse;
                    sendRequest("POST", "/api/v1/commands/ack", ackStr, ackResponse);
                }
                return true;
            }
        }
    }
    return false;
}

void IoTRestAPI::setCommandCallback(void (*callback)(const CommandData& cmd)) {
    _commandCallback = callback;
}

void IoTRestAPI::setConfigCallback(void (*callback)(const String& configJson)) {
    _configCallback = callback;
}

void IoTRestAPI::pollServer() {
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Polling server for updates...");
    #endif
    
    // Check for commands
    checkCommands();
    
    // Check for config updates
    String configJson;
    if (getConfig(configJson) && _configCallback) {
        _configCallback(configJson);
    }
}

bool IoTRestAPI::sendRequest(const String& method, const String& endpoint, const String& payload, String& response) {
    String url = _serverUrl + endpoint;
    
    _http.begin(url);
    addAuthHeader();
    _http.addHeader("Content-Type", "application/json");
    
    int httpCode = -1;
    
    if (method == "GET") {
        httpCode = _http.GET();
    } else if (method == "POST") {
        httpCode = _http.POST(payload);
    } else if (method == "PUT") {
        httpCode = _http.PUT(payload);
    } else if (method == "DELETE") {
        httpCode = _http.sendRequest("DELETE");
    }
    
    if (httpCode > 0) {
        response = _http.getString();
        
        #if ENABLE_SERIAL_DEBUG
        Serial.print("HTTP Response code: ");
        Serial.println(httpCode);
        Serial.print("Response: ");
        Serial.println(response);
        #endif
        
        _http.end();
        return (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED);
    } else {
        #if ENABLE_SERIAL_DEBUG
        Serial.print("HTTP Request failed: ");
        Serial.println(_http.errorToString(httpCode));
        #endif
        
        _http.end();
        return false;
    }
}

void IoTRestAPI::addAuthHeader() {
    String authHeader = "Bearer " + _deviceToken;
    _http.addHeader("Authorization", authHeader);
}