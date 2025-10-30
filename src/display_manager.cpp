// display_manager.cpp
#include "display_manager.h"
#include "config.h"
#include "pins.h"

DisplayManager::DisplayManager()
    : _display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, OLED_RESET),
      _currentScreen(SCREEN_MAIN),
      _lastUpdate(0),
      _lastActivity(0),
      _isDimmed(false),
      _messageEndTime(0),
      _setupPrompt("") {
}

bool DisplayManager::begin() {
    Wire.begin(DISPLAY_SDA, DISPLAY_SCL);
    
    if (!_display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("SSD1306 allocation failed");
        #endif
        return false;
    }
    
    _display.clearDisplay();
    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(0, 0);
    _display.println("Water Pump System");
    _display.println("Initializing...");
    _display.display();
    
    return true;
}

void DisplayManager::loop() {
    // Check for display timeout
    if (millis() - _lastActivity > (DISPLAY_TIMEOUT_SECONDS * 1000) && !_isDimmed) {
        dimDisplay();
    }
    
    // Handle temporary messages
    if (_messageEndTime > 0 && millis() > _messageEndTime) {
        _messageEndTime = 0;
        updateData(_data); // Refresh normal display
    }
    
    // Update display at regular intervals
    if (millis() - _lastUpdate > DISPLAY_UPDATE_INTERVAL_MS) {
        _lastUpdate = millis();

        if (_messageEndTime == 0) { // Only update if no temporary message
            switch (_currentScreen) {
                case SCREEN_MAIN:
                    drawMainScreen();
                    break;
                case SCREEN_STATUS:
                    drawStatusScreen();
                    break;
                case SCREEN_USAGE:
                    drawUsageScreen();
                    break;
                case SCREEN_SETUP:
                    drawSetupScreen(_setupPrompt);
                    break;
                default:
                    break;
            }
        }
    }
}

void DisplayManager::updateData(const DisplayData& data) {
    _data = data;
    _lastActivity = millis();
    if (_isDimmed) wakeDisplay();
}

void DisplayManager::setScreen(DisplayScreen screen) {
    _currentScreen = screen;
    _lastActivity = millis();
    if (_isDimmed) wakeDisplay();
}

DisplayScreen DisplayManager::getCurrentScreen() {
    return _currentScreen;
}

void DisplayManager::nextScreen() {
    int screen = (int)_currentScreen;
    screen = (screen + 1) % 3; // Cycle through 3 main screens
    _currentScreen = (DisplayScreen)screen;
    _lastActivity = millis();
}

void DisplayManager::previousScreen() {
    int screen = (int)_currentScreen;
    screen = (screen - 1 + 3) % 3;
    _currentScreen = (DisplayScreen)screen;
    _lastActivity = millis();
}

void DisplayManager::showMessage(const String& title, const String& message, int duration) {
    _display.clearDisplay();
    _display.setTextSize(1);
    
    // Draw title
    _display.setCursor(0, 0);
    _display.println(title);
    _display.drawLine(0, 10, DISPLAY_WIDTH, 10, SSD1306_WHITE);
    
    // Draw message (word wrap)
    _display.setCursor(0, 16);
    _display.println(message);
    
    _display.display();
    
    _messageTitle = title;
    _messageText = message;
    _messageEndTime = millis() + duration;
    _lastActivity = millis();
}

void DisplayManager::showProgress(const String& title, int percent) {
    _display.clearDisplay();
    _display.setTextSize(1);
    
    _display.setCursor(0, 0);
    _display.println(title);
    
    // Progress bar
    int barWidth = DISPLAY_WIDTH - 4;
    int barHeight = 16;
    int fillWidth = (barWidth * percent) / 100;
    
    _display.drawRect(2, 20, barWidth, barHeight, SSD1306_WHITE);
    _display.fillRect(3, 21, fillWidth - 2, barHeight - 2, SSD1306_WHITE);
    
    // Percentage text
    _display.setCursor(50, 45);
    _display.print(percent);
    _display.println("%");
    
    _display.display();
    _lastActivity = millis();
}

void DisplayManager::showError(const String& error) {
    showMessage("ERROR!", error, 5000);
}

void DisplayManager::showConfigMenu(int selectedItem) {
    drawConfigMenuScreen(selectedItem);
}

void DisplayManager::showSetupScreen(const String& prompt) {
    _setupPrompt = prompt;
    _currentScreen = SCREEN_SETUP;
    _lastActivity = millis();
    if (_isDimmed) wakeDisplay();
    drawSetupScreen(prompt);
}

void DisplayManager::setBrightness(uint8_t brightness) {
    _display.ssd1306_command(SSD1306_SETCONTRAST);
    _display.ssd1306_command(brightness);
}

void DisplayManager::dimDisplay() {
    setBrightness(0);
    _isDimmed = true;
}

void DisplayManager::wakeDisplay() {
    setBrightness(255);
    _isDimmed = false;
}

void DisplayManager::drawMainScreen() {
    _display.clearDisplay();
    
    // Title
    _display.setTextSize(1);
    _display.setCursor(0, 0);
    _display.print("Water Level: ");
    _display.print(formatFloat(_data.waterLevel));
    _display.println("%");
    
    // Tank visualization
    drawTankLevel(10, 12, 30, 48, _data.waterLevel);
    
    // Status info
    _display.setCursor(50, 12);
    _display.print("Pump: ");
    _display.println(_data.motorState ? "ON" : "OFF");
    
    _display.setCursor(50, 22);
    _display.print("Flow: ");
    _display.println(formatFloat(_data.currentInflow));
    
    _display.setCursor(50, 32);
    if (_data.overrideMode) {
        _display.println("OVERRIDE");
    } else if (_data.manualMode) {
        _display.println("MANUAL");
    } else {
        _display.println("AUTO");
    }
    
    // Alarms
    if (_data.dryRunAlarm) {
        _display.setCursor(50, 42);
        _display.println("DRY RUN!");
    }
    if (_data.overflowAlarm) {
        _display.setCursor(50, 52);
        _display.println("OVERFLOW!");
    }
    
    _display.display();
}

void DisplayManager::drawStatusScreen() {
    _display.clearDisplay();
    _display.setTextSize(1);
    
    _display.setCursor(0, 0);
    _display.println("=== STATUS ===");
    
    _display.setCursor(0, 12);
    _display.print("WiFi: ");
    _display.println(_data.wifiStatus);
    
    _display.setCursor(0, 22);
    _display.print("Cloud: ");
    _display.println(_data.iotStatus);
    
    _display.setCursor(0, 32);
    _display.print("Max Flow: ");
    _display.println(formatFloat(_data.maxInflow));
    
    _display.setCursor(0, 42);
    _display.print("Mode: ");
    if (_data.overrideMode) _display.println("OVERRIDE");
    else if (_data.manualMode) _display.println("MANUAL");
    else _display.println("AUTO");
    
    _display.display();
}

void DisplayManager::drawUsageScreen() {
    _display.clearDisplay();
    _display.setTextSize(1);
    
    _display.setCursor(0, 0);
    _display.println("=== USAGE ===");
    
    _display.setCursor(0, 16);
    _display.println("Today:");
    _display.setCursor(0, 26);
    _display.print(formatFloat(_data.dailyUsage, 2));
    _display.println(" L");
    
    _display.setCursor(0, 40);
    _display.println("This Month:");
    _display.setCursor(0, 50);
    _display.print(formatFloat(_data.monthlyUsage, 2));
    _display.println(" L");
    
    _display.display();
}

void DisplayManager::drawConfigMenuScreen(int selectedItem) {
    _display.clearDisplay();
    _display.setTextSize(1);
    
    _display.setCursor(0, 0);
    _display.println("=== CONFIG ===");
    
    const char* menuItems[] = {
        "Tank Height",
        "Tank Dimensions",
        "Thresholds",
        "WiFi Setup",
        "Factory Reset",
        "Exit"
    };
    
    for (int i = 0; i < 6; i++) {
        _display.setCursor(5, 12 + i * 10);
        if (i == selectedItem) {
            _display.print("> ");
        } else {
            _display.print("  ");
        }
        _display.println(menuItems[i]);
    }
    
    _display.display();
}

void DisplayManager::drawSetupScreen(const String& prompt) {
    _display.clearDisplay();
    _display.setTextSize(1);
    
    _display.setCursor(0, 0);
    _display.println("=== SETUP ===");
    
    _display.setCursor(0, 20);
    _display.println(prompt);
    
    _display.setCursor(0, 50);
    _display.println("Use buttons");
    
    _display.display();
}

void DisplayManager::drawTankLevel(int x, int y, int width, int height, float level) {
    // Draw tank outline
    _display.drawRect(x, y, width, height, SSD1306_WHITE);
    
    // Calculate fill height
    int fillHeight = (height - 2) * level / 100.0;
    int fillY = y + height - fillHeight - 1;
    
    // Draw water level
    if (fillHeight > 0) {
        _display.fillRect(x + 1, fillY, width - 2, fillHeight, SSD1306_WHITE);
    }
}

void DisplayManager::drawStatusIcon(int x, int y, bool state) {
    if (state) {
        _display.fillCircle(x, y, 3, SSD1306_WHITE);
    } else {
        _display.drawCircle(x, y, 3, SSD1306_WHITE);
    }
}

String DisplayManager::formatFloat(float value, int decimals) {
    return String(value, decimals);
}