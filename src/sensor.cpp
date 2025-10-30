// sensor.cpp
#include "sensor.h"
#include "config.h"
#include "pins.h"

UltrasonicSensor::UltrasonicSensor(uint8_t trigPin, uint8_t echoPin) 
    : _trigPin(trigPin), _echoPin(echoPin), _lastReadTime(0) {
}

bool UltrasonicSensor::begin() {
    pinMode(_trigPin, OUTPUT);
    pinMode(_echoPin, INPUT);
    digitalWrite(_trigPin, LOW);
    delay(50);
    
    // Test sensor
    float testDistance = getDistance();
    if (testDistance < 0) {
        _lastError = "Sensor initialization failed";
        return false;
    }
    
    return true;
}

float UltrasonicSensor::getDistance() {
    for (int retry = 0; retry < SENSOR_MAX_RETRIES; retry++) {
        float distance = readRaw();
        
        if (distance > 0 && distance < 400) { // JSN-SR04T max range ~400cm
            _lastError = "";
            return distance;
        }
        
        delay(50); // Small delay between retries
    }
    
    _lastError = "Sensor read timeout after retries";
    return -1.0;
}

float UltrasonicSensor::getAverageDistance(int samples) {
    float sum = 0;
    int validSamples = 0;
    
    for (int i = 0; i < samples; i++) {
        float distance = getDistance();
        if (distance > 0) {
            sum += distance;
            validSamples++;
        }
        delay(60); // 60ms between samples (JSN-SR04T minimum)
    }
    
    if (validSamples == 0) {
        _lastError = "No valid samples obtained";
        return -1.0;
    }
    
    return sum / validSamples;
}

bool UltrasonicSensor::isHealthy() {
    float distance = getDistance();
    return distance > 0;
}

String UltrasonicSensor::getLastError() {
    return _lastError;
}

float UltrasonicSensor::readRaw() {
    // Ensure minimum time between readings (60ms for JSN-SR04T)
    unsigned long currentTime = millis();
    if (currentTime - _lastReadTime < 60) {
        delay(60 - (currentTime - _lastReadTime));
    }
    
    // Send trigger pulse
    digitalWrite(_trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(_trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(_trigPin, LOW);
    
    // Read echo pulse
    unsigned long duration = pulseIn(_echoPin, HIGH, SENSOR_TIMEOUT_MS * 1000);
    _lastReadTime = millis();
    
    if (duration == 0) {
        return -1.0; // Timeout
    }
    
    // Calculate distance: duration in microseconds, speed of sound = 343 m/s
    // Distance = (duration * 0.0343) / 2
    float distance = (duration * 0.0343) / 2.0;
    
    return distance;
}