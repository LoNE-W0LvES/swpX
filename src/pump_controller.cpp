// pump_controller.cpp
#include "pump_controller.h"
#include "config.h"

PumpController::PumpController(uint8_t relayPin) 
    : _relayPin(relayPin),
      _pumpState(false),
      _mode(AUTO_MODE),
      _lastStateChangeTime(0),
      _currentCycleStartTime(0),
      _totalRunTime(0),
      _cycleCount(0),
      _dryRunDetected(false),
      _overflowRisk(false),
      _rapidCycleDetected(false),
      _dryRunCheckActive(false),
      _lastOnTime(0),
      _lastOffTime(0) {
}

void PumpController::begin() {
    pinMode(_relayPin, OUTPUT);
    digitalWrite(_relayPin, LOW); // Ensure pump is off initially
    _pumpState = false;
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Pump controller initialized");
    #endif
}

void PumpController::loop() {
    // Update total run time if pump is on
    if (_pumpState) {
        // Total run time is calculated on-demand in getTotalRunTime()
    }
}

void PumpController::turnOn(bool force) {
    setPumpState(true, force);
}

void PumpController::turnOff(bool force) {
    setPumpState(false, force);
}

bool PumpController::isOn() {
    return _pumpState;
}

void PumpController::setMode(PumpMode mode) {
    _mode = mode;
    
    #if ENABLE_SERIAL_DEBUG
    Serial.print("Pump mode set to: ");
    Serial.println(mode == AUTO_MODE ? "AUTO" : mode == MANUAL_MODE ? "MANUAL" : "OVERRIDE");
    #endif
}

PumpMode PumpController::getMode() {
    return _mode;
}

void PumpController::autoControl(float waterLevel, float upperThreshold, float lowerThreshold) {
    if (_mode != AUTO_MODE) return;
    
    // Turn on pump if below lower threshold
    if (waterLevel <= lowerThreshold && !_pumpState) {
        if (canTurnOn(waterLevel)) {
            turnOn();
        }
    }
    // Turn off pump if above upper threshold
    else if (waterLevel >= upperThreshold && _pumpState) {
        if (canTurnOff()) {
            turnOff();
        }
    }
}

void PumpController::toggleManual() {
    if (_mode == MANUAL_MODE) {
        setPumpState(!_pumpState, false);
    }
}

void PumpController::enterOverrideMode() {
    _mode = OVERRIDE_MODE;
    resetSafetyAlarms();
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Override mode ACTIVATED - Safety features disabled!");
    #endif
}

void PumpController::exitOverrideMode() {
    _mode = AUTO_MODE;
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Override mode deactivated - Returning to AUTO mode");
    #endif
}

void PumpController::updateSafetyCheck(float currentLevel, float previousLevel, unsigned long deltaTimeMs) {
    if (_mode == OVERRIDE_MODE) return; // Skip safety checks in override mode
    
    // Dry run detection
    #if ENABLE_DRY_RUN_PROTECTION
    if (_pumpState) {
        if (!_dryRunCheckActive) {
            _dryRunCheckActive = true;
            _dryRunCheckStartTime = millis();
            _dryRunCheckStartLevel = currentLevel;
        } else {
            unsigned long runTime = millis() - _dryRunCheckStartTime;
            float levelIncrease = currentLevel - _dryRunCheckStartLevel;
            
            // If pump has been running for DRY_RUN_TIMEOUT and level hasn't increased
            if (runTime > (DRY_RUN_TIMEOUT_MINUTES * 60000)) {
                if (levelIncrease < 1.0) { // Less than 1% increase
                    _dryRunDetected = true;
                    turnOff(true); // Force off
                    
                    #if ENABLE_SERIAL_DEBUG
                    Serial.println("DRY RUN DETECTED! Pump stopped.");
                    #endif
                }
                // Reset check for next cycle
                _dryRunCheckActive = false;
            }
        }
    } else {
        _dryRunCheckActive = false;
    }
    #endif
    
    // Overflow protection
    #if ENABLE_OVERFLOW_PROTECTION
    if (currentLevel >= OVERFLOW_EMERGENCY_LEVEL) {
        _overflowRisk = true;
        if (_pumpState) {
            turnOff(true); // Force off
            
            #if ENABLE_SERIAL_DEBUG
            Serial.println("OVERFLOW RISK! Pump stopped.");
            #endif
        }
    } else if (currentLevel < (OVERFLOW_EMERGENCY_LEVEL - 5.0)) {
        // Reset overflow flag when level drops 5% below emergency level
        _overflowRisk = false;
    }
    #endif
}

bool PumpController::isDryRunDetected() {
    return _dryRunDetected;
}

bool PumpController::isOverflowRisk() {
    return _overflowRisk;
}

bool PumpController::isRapidCycleDetected() {
    return _rapidCycleDetected;
}

unsigned long PumpController::getTotalRunTime() {
    unsigned long total = _totalRunTime;
    if (_pumpState && _currentCycleStartTime > 0) {
        total += (millis() - _currentCycleStartTime);
    }
    return total;
}

unsigned long PumpController::getCurrentRunTime() {
    if (_pumpState && _currentCycleStartTime > 0) {
        return millis() - _currentCycleStartTime;
    }
    return 0;
}

int PumpController::getCycleCount() {
    return _cycleCount;
}

void PumpController::resetSafetyAlarms() {
    _dryRunDetected = false;
    _overflowRisk = false;
    _rapidCycleDetected = false;
    _dryRunCheckActive = false;
}

unsigned long PumpController::getLastStateChangeTime() {
    return _lastStateChangeTime;
}

void PumpController::setPumpState(bool state, bool force) {
    if (state == _pumpState && !force) return; // Already in desired state
    
    if (state) {
        // Turning ON
        if (!force && !canTurnOn(0)) return;
        
        digitalWrite(_relayPin, HIGH);
        _pumpState = true;
        _currentCycleStartTime = millis();
        _lastOnTime = millis();
        _lastStateChangeTime = millis();
        _cycleCount++;
        
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Pump turned ON");
        #endif
    } else {
        // Turning OFF
        if (!force && !canTurnOff()) return;
        
        digitalWrite(_relayPin, LOW);
        _pumpState = false;
        _lastOffTime = millis();
        _lastStateChangeTime = millis();
        
        // Update total run time
        if (_currentCycleStartTime > 0) {
            _totalRunTime += (millis() - _currentCycleStartTime);
            _currentCycleStartTime = 0;
        }
        
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Pump turned OFF");
        #endif
    }
}

bool PumpController::canTurnOn(float waterLevel) {
    if (_mode == OVERRIDE_MODE) return true; // Override bypasses all checks
    
    // Check overflow protection
    #if ENABLE_OVERFLOW_PROTECTION
    if (_overflowRisk) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Cannot turn on: Overflow risk");
        #endif
        return false;
    }
    
    if (waterLevel >= MANUAL_OVERRIDE_MAX_LEVEL) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Cannot turn on: Water level too high");
        #endif
        return false;
    }
    #endif
    
    // Check rapid cycle protection
    #if ENABLE_RAPID_CYCLE_PROTECTION
    unsigned long timeSinceLastOff = millis() - _lastOffTime;
    if (_lastOffTime > 0 && timeSinceLastOff < (MINIMUM_OFF_TIME_SECONDS * 1000)) {
        _rapidCycleDetected = true;
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Cannot turn on: Minimum off time not elapsed");
        #endif
        return false;
    }
    _rapidCycleDetected = false;
    #endif
    
    // Check dry run
    #if ENABLE_DRY_RUN_PROTECTION
    if (_dryRunDetected) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Cannot turn on: Dry run detected");
        #endif
        return false;
    }
    #endif
    
    return true;
}

bool PumpController::canTurnOff() {
    if (_mode == OVERRIDE_MODE) return true; // Override bypasses all checks
    
    // Check minimum run time
    #if ENABLE_RAPID_CYCLE_PROTECTION
    unsigned long runTime = millis() - _lastOnTime;
    if (runTime < (MINIMUM_RUN_TIME_SECONDS * 1000)) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Cannot turn off: Minimum run time not elapsed");
        #endif
        return false;
    }
    #endif
    
    return true;
}