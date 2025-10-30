// pins.h - ESP32-S3 Pin Definitions - FIXED VERSION
#ifndef PINS_H
#define PINS_H

// ==================== BUTTON PINS ====================
// Matches diagram.json exactly
#define BTN_TOP     13      // btn1 - Top navigation button
#define BTN_MID     15      // btn3 - Middle button (enter/select)  
#define BTN_BOTTOM  14      // btn2 - Bottom navigation button
#define BTN_LEFT    12      // btn4 - Left navigation button
#define BTN_RIGHT   11      // btn5 - Right navigation button

// ==================== MANUAL SWITCH ====================
// ✅ FIXED: Now using dedicated 6th button (btn6 in diagram.json)
#define MANUAL_SWITCH_PIN 16  // Dedicated manual switch on GPIO 16 (btn6)

// ==================== PUMP CONTROL ====================
#define PUMP_RELAY_PIN 10   // Relay control pin (matches diagram.json)

// ==================== ULTRASONIC SENSOR (JSN-SR04T) ====================
#define SENSOR_TRIG_PIN 4   // Trigger pin (matches diagram.json)
#define SENSOR_ECHO_PIN 5   // Echo pin (matches diagram.json)

// ==================== I2C DISPLAY (SSD1306) ====================
#define DISPLAY_SDA 8       // I2C Data (matches diagram.json)
#define DISPLAY_SCL 9       // I2C Clock (matches diagram.json)
#define DISPLAY_WIDTH 128   // OLED display width in pixels
#define DISPLAY_HEIGHT 64   // OLED display height in pixels
#define OLED_RESET -1       // Reset pin (or -1 if sharing ESP reset pin)
#define SCREEN_ADDRESS 0x3C // I2C address for SSD1306 (matches diagram.json)

// ==================== STATUS LED (Optional) ====================
#define STATUS_LED_PIN 48   // Built-in LED or external status LED

// ==================== RESERVED PINS (DO NOT USE) ====================
// GPIO 19, 20: USB D- and D+
// GPIO 26-32: Reserved for PSRAM
// GPIO 33-37: Reserved for Flash
// GPIO 0: Boot button (available but be careful)

// ==================== AVAILABLE PINS FOR EXPANSION ====================
// GPIO 1, 2, 3, 6, 7, 16, 17, 18, 21 are available

#endif // PINS_H