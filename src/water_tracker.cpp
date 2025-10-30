// water_tracker.cpp
#include "water_tracker.h"
#include "config.h"

WaterTracker::WaterTracker() 
    : _storage(nullptr),
      _calculator(nullptr),
      _currentLevel(0),
      _previousLevel(0),
      _currentPumpState(false),
      _previousPumpState(false),
      _lastUpdateTime(0),
      _lastMidnightCheck(0),
      _todayUsageLiters(0),
      _todayCycles(0),
      _todayStartTimestamp(0) {
}

void WaterTracker::begin(StorageManager* storage, TankCalculator* calculator) {
    _storage = storage;
    _calculator = calculator;
    
    // Load today's data if exists
    unsigned long todayTimestamp = getMidnightTimestamp();
    DailyUsage todayData;
    
    if (_storage->getDailyUsage(todayTimestamp, todayData)) {
        _todayUsageLiters = todayData.totalUsageLiters;
        _todayCycles = todayData.pumpCycles;
        _todayStartTimestamp = todayData.date;
    } else {
        _todayStartTimestamp = todayTimestamp;
    }
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Water tracker initialized");
    Serial.print("Today's usage so far: ");
    Serial.print(_todayUsageLiters);
    Serial.println(" L");
    #endif
}

void WaterTracker::loop() {
    // Check if it's past midnight
    if (millis() - _lastMidnightCheck > 60000) { // Check every minute
        _lastMidnightCheck = millis();
        if (isMidnight()) {
            resetDaily();
        }
    }
}

void WaterTracker::updateState(float waterLevel, bool pumpState, float currentInflow) {
    _previousLevel = _currentLevel;
    _previousPumpState = _currentPumpState;
    
    _currentLevel = waterLevel;
    _currentPumpState = pumpState;
    
    // Detect pump state changes
    if (pumpState && !_previousPumpState) {
        // Pump turned ON
        _todayCycles++;
        
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Pump cycle detected");
        #endif
    }
    
    // Detect water usage (level decrease when pump is off)
    if (!pumpState && _previousLevel > _currentLevel) {
        float levelDrop = _previousLevel - _currentLevel;
        float volumeUsed = _calculator->levelToVolume(levelDrop);
        
        if (volumeUsed > 0 && volumeUsed < 100) { // Sanity check (less than 100L at once)
            _todayUsageLiters += volumeUsed;
            
            #if ENABLE_SERIAL_DEBUG
            Serial.print("Water usage detected: ");
            Serial.print(volumeUsed);
            Serial.println(" L");
            #endif
            
            // Save updated daily data
            saveDailyData();
        }
    }
    
    _lastUpdateTime = millis();
}

float WaterTracker::getTodayUsage() {
    return _todayUsageLiters;
}

float WaterTracker::getMonthUsage() {
    if (!_storage) return 0;
    
    // Get current date
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    float totalUsage = 0;
    _storage->getMonthlyUsage(timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, totalUsage);
    
    return totalUsage;
}

int WaterTracker::getTodayCycles() {
    return _todayCycles;
}

bool WaterTracker::getLast30Days(DailyUsage* usageArray, int& count) {
    if (!_storage) return false;
    
    return _storage->getLast30DaysUsage(usageArray, count);
}

void WaterTracker::resetDaily() {
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Midnight detected - Resetting daily usage");
    Serial.print("Total usage yesterday: ");
    Serial.print(_todayUsageLiters);
    Serial.println(" L");
    #endif
    
    // Save final data for the day
    saveDailyData();
    
    // Reset counters
    _todayUsageLiters = 0;
    _todayCycles = 0;
    _todayStartTimestamp = getMidnightTimestamp();
    
    // Save new empty daily record
    saveDailyData();
}

void WaterTracker::resetAll() {
    _todayUsageLiters = 0;
    _todayCycles = 0;
    _todayStartTimestamp = getMidnightTimestamp();
    saveDailyData();
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Water usage tracker reset");
    #endif
}

void WaterTracker::detectUsage() {
    // This is called periodically to detect usage patterns
    // Can be extended for ML feature extraction
}

void WaterTracker::saveDailyData() {
    if (!_storage) return;
    
    DailyUsage todayData;
    todayData.date = _todayStartTimestamp;
    todayData.totalUsageLiters = _todayUsageLiters;
    todayData.pumpCycles = _todayCycles;
    
    _storage->saveDailyUsage(todayData);
}

bool WaterTracker::isMidnight() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    // Check if it's within 1 minute of midnight
    if (timeinfo->tm_hour == 0 && timeinfo->tm_min == 0) {
        // Make sure we only trigger once per day
        unsigned long todayMidnight = getMidnightTimestamp();
        if (todayMidnight != _todayStartTimestamp) {
            return true;
        }
    }
    
    return false;
}

unsigned long WaterTracker::getMidnightTimestamp() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    // Set to midnight
    timeinfo->tm_hour = 0;
    timeinfo->tm_min = 0;
    timeinfo->tm_sec = 0;
    
    return mktime(timeinfo);
}

unsigned long WaterTracker::getCurrentTimestamp() {
    return time(nullptr);
}