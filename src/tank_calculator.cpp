// tank_calculator.cpp
#include "tank_calculator.h"
#include "config.h"
#include <math.h>

TankCalculator::TankCalculator() : _tankCapacityLiters(0) {}

void TankCalculator::setTankConfig(const TankConfig& config) {
    _config = config;
    calculateCapacity();
}

float TankCalculator::distanceToLevel(float sensorDistance) {
    if (sensorDistance < 0 || _config.tankHeight <= 0) {
        return 0.0;
    }
    
    // Subtract dead zone from sensor reading
    float adjustedDistance = sensorDistance - SENSOR_DEAD_ZONE_CM;
    
    // Calculate water height (distance from bottom of tank)
    float waterHeight = _config.tankHeight - adjustedDistance;
    
    // Clamp to valid range
    if (waterHeight < 0) waterHeight = 0;
    if (waterHeight > _config.tankHeight) waterHeight = _config.tankHeight;
    
    // Convert to percentage
    float levelPercent = (waterHeight / _config.tankHeight) * 100.0;
    
    return levelPercent;
}

float TankCalculator::levelToVolume(float levelPercent) {
    if (levelPercent < 0) levelPercent = 0;
    if (levelPercent > 100) levelPercent = 100;
    
    return (_tankCapacityLiters * levelPercent) / 100.0;
}

float TankCalculator::distanceToVolume(float sensorDistance) {
    float levelPercent = distanceToLevel(sensorDistance);
    return levelToVolume(levelPercent);
}

float TankCalculator::distanceToWaterHeight(float sensorDistance) {
    float adjustedDistance = sensorDistance - SENSOR_DEAD_ZONE_CM;
    float waterHeight = _config.tankHeight - adjustedDistance;
    
    if (waterHeight < 0) waterHeight = 0;
    if (waterHeight > _config.tankHeight) waterHeight = _config.tankHeight;
    
    return waterHeight;
}

float TankCalculator::calculateInflow(float currentLevel, float previousLevel, unsigned long deltaTimeMs) {
    if (deltaTimeMs == 0) return 0.0;
    
    // Calculate level change
    float levelChange = currentLevel - previousLevel;
    
    // Convert level change to volume change
    float volumeChange = (_tankCapacityLiters * levelChange) / 100.0;
    
    // Convert liters to cm³
    float volumeChangeCm3 = volumeChange * 1000.0;
    
    // Convert time to seconds
    float deltaTimeSec = deltaTimeMs / 1000.0;
    
    // Calculate flow rate (cm³/sec)
    float inflow = volumeChangeCm3 / deltaTimeSec;
    
    return inflow;
}

float TankCalculator::getTankCapacity() {
    return _tankCapacityLiters;
}

bool TankCalculator::isConfigValid() {
    if (_config.tankHeight <= 0) return false;
    
    if (_config.shape == RECTANGULAR) {
        if (_config.tankLength <= 0 || _config.tankWidth <= 0) return false;
    } else if (_config.shape == CYLINDRICAL) {
        if (_config.tankRadius <= 0) return false;
    }
    
    if (_config.upperThreshold <= _config.lowerThreshold) return false;
    if (_config.upperThreshold > 100 || _config.lowerThreshold < 0) return false;
    
    return true;
}

void TankCalculator::calculateCapacity() {
    float volumeCm3 = 0;
    
    if (_config.shape == RECTANGULAR) {
        // Volume = length × width × height
        volumeCm3 = _config.tankLength * _config.tankWidth * _config.tankHeight;
    } else if (_config.shape == CYLINDRICAL) {
        // Volume = π × r² × height
        volumeCm3 = M_PI * _config.tankRadius * _config.tankRadius * _config.tankHeight;
    }
    
    // Convert cm³ to liters (1 liter = 1000 cm³)
    _tankCapacityLiters = volumeCm3 / 1000.0;
}