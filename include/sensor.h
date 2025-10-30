// sensor.h
#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>

class UltrasonicSensor {
public:
    UltrasonicSensor(uint8_t trigPin, uint8_t echoPin);
    bool begin();
    
    // Get distance in centimeters (returns -1 on error)
    float getDistance();
    
    // Get distance with multiple samples and averaging
    float getAverageDistance(int samples = 3);
    
    // Check if sensor is working
    bool isHealthy();
    
    // Get last error message
    String getLastError();
    
private:
    uint8_t _trigPin;
    uint8_t _echoPin;
    String _lastError;
    unsigned long _lastReadTime;
    
    float readRaw();
};

#endif // SENSOR_H