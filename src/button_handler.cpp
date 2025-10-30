// button_handler.cpp
#include "button_handler.h"
#include "config.h"
#include "pins.h"

ButtonHandler::ButtonHandler() 
    : _lastEvent(BTN_NONE), _eventAvailable(false) {
}

void ButtonHandler::begin() {
    // Initialize all button pins with internal pull-up
    pinMode(BTN_TOP, INPUT_PULLUP);
    pinMode(BTN_MID, INPUT_PULLUP);
    pinMode(BTN_BOTTOM, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(MANUAL_SWITCH_PIN, INPUT_PULLUP);
    
    // Initialize button states
    _btnTop = {false, false, 0, 0, false};
    _btnMid = {false, false, 0, 0, false};
    _btnBottom = {false, false, 0, 0, false};
    _btnLeft = {false, false, 0, 0, false};
    _btnRight = {false, false, 0, 0, false};
    _btnManual = {false, false, 0, 0, false};
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Button handler initialized");
    #endif
}

void ButtonHandler::loop() {
    // Update all buttons
    updateButton(_btnTop, BTN_TOP, BTN_TOP_PRESS);
    updateButton(_btnMid, BTN_MID, BTN_MID_PRESS, BTN_MID_LONG_PRESS);
    updateButton(_btnBottom, BTN_BOTTOM, BTN_BOTTOM_PRESS);
    updateButton(_btnLeft, BTN_LEFT, BTN_LEFT_PRESS);
    updateButton(_btnRight, BTN_RIGHT, BTN_RIGHT_PRESS);
    updateButton(_btnManual, MANUAL_SWITCH_PIN, BTN_MANUAL_SWITCH_TOGGLE, BTN_MANUAL_SWITCH_LONG_PRESS);
}

ButtonEvent ButtonHandler::getEvent() {
    if (_eventAvailable) {
        _eventAvailable = false;
        ButtonEvent event = _lastEvent;
        _lastEvent = BTN_NONE;
        return event;
    }
    return BTN_NONE;
}

bool ButtonHandler::isPressed(uint8_t pin) {
    return digitalRead(pin) == LOW; // Active low with pull-up
}

bool ButtonHandler::getManualSwitchState() {
    return _btnManual.currentState;
}

void ButtonHandler::resetLongPress() {
    _btnMid.longPressTriggered = false;
    _btnManual.longPressTriggered = false;
}

void ButtonHandler::updateButton(ButtonState& btn, uint8_t pin, ButtonEvent shortEvent, ButtonEvent longEvent) {
    bool reading = readDebounced(btn, pin);
    
    // Detect press (transition from released to pressed)
    if (reading && !btn.lastState) {
        btn.pressStartTime = millis();
        btn.longPressTriggered = false;
    }
    
    // Detect release (transition from pressed to released)
    if (!reading && btn.lastState) {
        unsigned long pressDuration = millis() - btn.pressStartTime;
        
        // Only trigger short press if long press wasn't already triggered
        if (!btn.longPressTriggered && pressDuration < BUTTON_LONG_PRESS_MS) {
            _lastEvent = shortEvent;
            _eventAvailable = true;
            
            #if ENABLE_SERIAL_DEBUG
            Serial.print("Button event: ");
            Serial.println(shortEvent);
            #endif
        }
        
        btn.longPressTriggered = false;
    }
    
    // Check for long press while button is held
    if (reading && !btn.longPressTriggered && longEvent != BTN_NONE) {
        unsigned long pressDuration = millis() - btn.pressStartTime;
        
        if (pressDuration >= BUTTON_LONG_PRESS_MS) {
            btn.longPressTriggered = true;
            _lastEvent = longEvent;
            _eventAvailable = true;
            
            #if ENABLE_SERIAL_DEBUG
            Serial.print("Long press event: ");
            Serial.println(longEvent);
            #endif
        }
    }
    
    btn.lastState = reading;
    btn.currentState = reading;
}

bool ButtonHandler::readDebounced(ButtonState& btn, uint8_t pin) {
    bool reading = isPressed(pin);
    
    // If the reading has changed, reset debounce timer
    if (reading != btn.lastState) {
        btn.lastDebounceTime = millis();
    }
    
    // Only return stable reading after debounce time
    if ((millis() - btn.lastDebounceTime) > BUTTON_DEBOUNCE_MS) {
        return reading;
    }
    
    return btn.currentState; // Return previous stable state
}