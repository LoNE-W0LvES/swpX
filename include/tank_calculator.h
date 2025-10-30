// tank_calculator.h
#ifndef TANK_CALCULATOR_H
#define TANK_CALCULATOR_H

#include "storage_manager.h"

class TankCalculator {
public:
    TankCalculator();
    
    void setTankConfig(const TankConfig& config);
    
    // Convert sensor distance to water level percentage
    float distanceToLevel(float sensorDistance);
    
    // Convert water level percentage to volume in liters
    float levelToVolume(float levelPercent);
    
    // Convert distance to volume in liters
    float distanceToVolume(float sensorDistance);
    
    // Calculate water height from sensor distance
    float distanceToWaterHeight(float sensorDistance);
    
    // Calculate current inflow rate (cm³/sec)
    // Positive = filling, Negative = draining, 0 = stable
    float calculateInflow(float currentLevel, float previousLevel, unsigned long deltaTimeMs);
    
    // Get tank capacity in liters
    float getTankCapacity();
    
    // Validate tank configuration
    bool isConfigValid();
    
private:
    TankConfig _config;
    float _tankCapacityLiters;
    
    void calculateCapacity();
};

#endif // TANK_CALCULATOR_H

