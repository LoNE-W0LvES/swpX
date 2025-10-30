// iot_client.h
#ifndef IOT_CLIENT_H
#define IOT_CLIENT_H

#include "config.h"
#include "storage_manager.h"

// Define protocol types as numbers
#define PROTOCOL_MQTT 1
#define PROTOCOL_WEBSOCKET 2
#define PROTOCOL_RESTAPI 3

#if COMMUNICATION_PROTOCOL == PROTOCOL_MQTT
    #include "iot_mqtt.h"
    typedef IoTMQTT IoTProtocol;
#elif COMMUNICATION_PROTOCOL == PROTOCOL_WEBSOCKET
    #include "iot_websocket.h"
    typedef IoTWebSocket IoTProtocol;
#elif COMMUNICATION_PROTOCOL == PROTOCOL_RESTAPI
    #include "iot_restapi.h"
    typedef IoTRestAPI IoTProtocol;
#endif

// Unified telemetry structure (already defined in protocol files, but redefined here for clarity)
// struct TelemetryData {
//     unsigned long timestamp;
//     bool motorState;
//     float waterLevel;
//     float currentInflow;
//     float maxInflow;
//     float dailyUsage;
//     float monthlyUsage;
// };

// struct CommandData {
//     String command;
//     String payload;
// };

class IoTClient {
public:
    IoTClient();
    
    // Initialize with device token from storage
    bool begin();
    
    // Must be called in loop()
    void loop();
    
    // Connection status
    bool isConnected();
    
    // Send data to cloud
    bool sendTelemetry(const TelemetryData& data);
    bool sendStatus(const String& status);
    bool sendConfig(const TankConfig& config);
    
    // Request config from cloud
    bool requestConfig();
    
    // Set callbacks for cloud commands and config updates
    void setCommandCallback(void (*callback)(const CommandData& cmd));
    void setConfigCallback(void (*callback)(const String& configJson));
    
    // Get protocol name
    String getProtocolName();
    
private:
    IoTProtocol _protocol;
    StorageManager _storage;
    String _deviceToken;
    unsigned long _lastTelemetrySend;
    bool _initialized;
};

#endif // IOT_CLIENT_H