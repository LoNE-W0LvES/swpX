// display_manager.h
#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

enum DisplayScreen {
    SCREEN_MAIN,
    SCREEN_STATUS,
    SCREEN_USAGE,
    SCREEN_CONFIG_MENU,
    SCREEN_SETUP
};

struct DisplayData {
    float waterLevel;
    float currentInflow;
    float maxInflow;
    bool motorState;
    bool manualMode;
    bool overrideMode;
    float dailyUsage;
    float monthlyUsage;
    String wifiStatus;
    String iotStatus;
    bool dryRunAlarm;
    bool overflowAlarm;
};

class DisplayManager {
public:
    DisplayManager();
    
    bool begin();
    void loop();
    
    // Update display data
    void updateData(const DisplayData& data);
    
    // Screen management
    void setScreen(DisplayScreen screen);
    DisplayScreen getCurrentScreen();
    void nextScreen();
    void previousScreen();
    
    // Display messages
    void showMessage(const String& title, const String& message, int duration = 2000);
    void showProgress(const String& title, int percent);
    void showError(const String& error);
    
    // Configuration menu
    void showConfigMenu(int selectedItem);
    void showSetupScreen(const String& prompt);
    
    // Brightness control
    void setBrightness(uint8_t brightness);
    void dimDisplay();
    void wakeDisplay();
    
private:
    Adafruit_SSD1306 _display;
    DisplayData _data;
    DisplayScreen _currentScreen;
    unsigned long _lastUpdate;
    unsigned long _lastActivity;
    bool _isDimmed;
    unsigned long _messageEndTime;
    String _messageTitle;
    String _messageText;
    
    void drawMainScreen();
    void drawStatusScreen();
    void drawUsageScreen();
    void drawConfigMenuScreen(int selectedItem);
    void drawSetupScreen(const String& prompt);
    
    void drawTankLevel(int x, int y, int width, int height, float level);
    void drawStatusIcon(int x, int y, bool state);
    String formatFloat(float value, int decimals = 1);
};

#endif // DISPLAY_MANAGER_H

