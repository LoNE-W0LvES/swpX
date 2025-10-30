// button_handler.h
#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>

enum ButtonEvent {
    BTN_NONE,
    BTN_TOP_PRESS,
    BTN_MID_PRESS,
    BTN_BOTTOM_PRESS,
    BTN_LEFT_PRESS,
    BTN_RIGHT_PRESS,
    BTN_MID_LONG_PRESS,
    BTN_MANUAL_SWITCH_TOGGLE,
    BTN_MANUAL_SWITCH_LONG_PRESS
};

class ButtonHandler {
public:
    ButtonHandler();
    
    void begin();
    void loop();
    
    // Get the last button event (consumes the event)
    ButtonEvent getEvent();
    
    // Check if a specific button is currently pressed
    bool isPressed(uint8_t pin);
    
    // Check manual switch state
    bool getManualSwitchState();
    
    // Reset long press timer
    void resetLongPress();
    
private:
    // Button states
    struct ButtonState {
        bool currentState;
        bool lastState;
        unsigned long lastDebounceTime;
        unsigned long pressStartTime;
        bool longPressTriggered;
    };
    
    ButtonState _btnTop;
    ButtonState _btnMid;
    ButtonState _btnBottom;
    ButtonState _btnLeft;
    ButtonState _btnRight;
    ButtonState _btnManual;
    
    ButtonEvent _lastEvent;
    bool _eventAvailable;
    
    void updateButton(ButtonState& btn, uint8_t pin, ButtonEvent shortEvent, ButtonEvent longEvent = BTN_NONE);
    bool readDebounced(ButtonState& btn, uint8_t pin);
};

#endif // BUTTON_HANDLER_H
