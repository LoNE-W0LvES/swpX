// water_tracker.h
#ifndef WATER_TRACKER_H
#define WATER_TRACKER_H

#include "storage_manager.h"
#include "tank_calculator.h"
#include <time.h>

struct UsageSnapshot {
    unsigned long timestamp;
    float waterLevel;
    float volumeLiters;
    bool pumpState;
};

class WaterTracker {
public:
    WaterTracker();
    
    void begin(StorageManager* storage, TankCalculator* calculator);
    void loop();
    
    // Update current state
    void updateState(float waterLevel, bool pumpState, float currentInflow);
    
    // Get usage statistics
    float getTodayUsage();        // Liters used today
    float getMonthUsage();        // Liters used this month
    int getTodayCycles();         // Pump on/off cycles today
    
    // Get historical data
    bool getLast30Days(DailyUsage* usageArray, int& count);
    
    // Reset daily counter (called at midnight)
    void resetDaily();
    
    // Manual reset
    void resetAll();
    
private:
    StorageManager* _storage;
    TankCalculator* _calculator;
    
    // Current tracking
    float _currentLevel;
    float _previousLevel;
    bool _currentPumpState;
    bool _previousPumpState;
    unsigned long _lastUpdateTime;
    unsigned long _lastMidnightCheck;
    
    // Daily accumulation
    float _todayUsageLiters;
    int _todayCycles;
    unsigned long _todayStartTimestamp;
    
    // Snapshot for change detection
    UsageSnapshot _lastSnapshot;
    
    // Helper functions
    void detectUsage();
    void saveDailyData();
    bool isMidnight();
    unsigned long getMidnightTimestamp();
    unsigned long getCurrentTimestamp();
};

#endif // WATER_TRACKER_H

