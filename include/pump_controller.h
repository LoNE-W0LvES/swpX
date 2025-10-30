// pump_controller.h
#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include <Arduino.h>
#include "storage_manager.h"

enum PumpMode {
    AUTO_MODE,
    MANUAL_MODE,
    OVERRIDE_MODE
};

class PumpController {
public:
    PumpController(uint8_t relayPin);
    
    void begin();
    void loop();
    
    // Pump control
    void turnOn(bool force = false);
    void turnOff(bool force = false);
    bool isOn();
    
    // Mode management
    void setMode(PumpMode mode);
    PumpMode getMode();
    
    // Automatic control based on water level
    void autoControl(float waterLevel, float upperThreshold, float lowerThreshold);
    
    // Manual control (toggle)
    void toggleManual();
    
    // Override mode (hold for 5 seconds)
    void enterOverrideMode();
    void exitOverrideMode();
    
    // Safety features
    void updateSafetyCheck(float currentLevel, float previousLevel, unsigned long deltaTimeMs);
    bool isDryRunDetected();
    bool isOverflowRisk();
    bool isRapidCycleDetected();
    
    // Get pump statistics
    unsigned long getTotalRunTime();
    unsigned long getCurrentRunTime();
    int getCycleCount();
    
    // Reset safety alarms
    void resetSafetyAlarms();
    
    // Get last state change time
    unsigned long getLastStateChangeTime();
    
private:
    uint8_t _relayPin;
    bool _pumpState;
    PumpMode _mode;
    
    // Timing
    unsigned long _lastStateChangeTime;
    unsigned long _currentCycleStartTime;
    unsigned long _totalRunTime;
    int _cycleCount;
    
    // Safety flags
    bool _dryRunDetected;
    bool _overflowRisk;
    bool _rapidCycleDetected;
    
    // Dry run detection
    unsigned long _dryRunCheckStartTime;
    float _dryRunCheckStartLevel;
    bool _dryRunCheckActive;
    
    // Rapid cycle protection
    unsigned long _lastOnTime;
    unsigned long _lastOffTime;
    
    // Internal control
    void setPumpState(bool state, bool force);
    bool canTurnOn(float waterLevel);
    bool canTurnOff();
};

#endif // PUMP_CONTROLLER_H

