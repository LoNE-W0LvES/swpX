// iot_types.h - Common IoT data structures
#ifndef IOT_TYPES_H
#define IOT_TYPES_H

#include <Arduino.h>

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

#endif // IOT_TYPES_H
