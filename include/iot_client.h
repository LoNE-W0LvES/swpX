// iot_client.h
#ifndef IOT_CLIENT_H
#define IOT_CLIENT_H

#include "config.h"
#include "storage_manager.h"
#include "iot_types.h"

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

class IoTClient {
public:
    IoTClient();
    
    bool begin();
    void loop();
    bool isConnected();
    
    bool sendTelemetry(const TelemetryData& data);
    bool sendStatus(const String& status);
    bool sendConfig(const TankConfig& config);
    bool requestConfig();
    
    void setCommandCallback(void (*callback)(const CommandData& cmd));
    void setConfigCallback(void (*callback)(const String& configJson));
    
    String getProtocolName();
    
private:
    IoTProtocol _protocol;
    StorageManager _storage;
    String _deviceToken;
    unsigned long _lastTelemetrySend;
    bool _initialized;
};

#endif // IOT_CLIENT_H
