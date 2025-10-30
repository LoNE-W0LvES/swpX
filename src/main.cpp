// main.cpp - Smart Water Pump System - FIXED SCREEN FLICKERING
#include <Arduino.h>
#include "config.h"
#include "pins.h"
#include "storage_manager.h"
#include "sensor.h"
#include "tank_calculator.h"
#include "pump_controller.h"
#include "display_manager.h"
#include "button_handler.h"
#include "wifi_manager.h"
#include "iot_client.h"
#include "sync_manager.h"
#include "water_tracker.h"
#include "webserver_local.h"
#include "ota_updater.h"
#include "ml_predictor.h"
#include "utils.h"

// ==================== GLOBAL OBJECTS ====================
StorageManager storage;
UltrasonicSensor sensor(SENSOR_TRIG_PIN, SENSOR_ECHO_PIN);
TankCalculator calculator;
PumpController pumpController(PUMP_RELAY_PIN);
DisplayManager displayManager;
ButtonHandler buttonHandler;
WiFiManager wifiManager;
IoTClient iotClient;
SyncManager syncManager;
WaterTracker waterTracker;
WebServerLocal webServer;
OTAUpdater otaUpdater;
MLPredictor mlPredictor;

// ==================== GLOBAL STATE ====================
TankConfig currentConfig;
float currentWaterLevel = 0.0;
float previousWaterLevel = 0.0;
float currentInflow = 0.0;
float maxInflow = 0.0;
unsigned long lastSensorRead = 0;
unsigned long lastTelemetrySend = 0;
bool systemInitialized = false;
bool wifiInitialized = false;  // Track if TCP/IP stack is ready

enum SystemState {
    STATE_FIRST_TIME_SETUP,
    STATE_NORMAL_OPERATION,
    STATE_CONFIG_MODE,
    STATE_ERROR
};

SystemState systemState = STATE_FIRST_TIME_SETUP;

// ==================== FUNCTION DECLARATIONS ====================
void initializeSystem();
void firstTimeSetup();
void normalOperation();
void configMode();
void handleButtonEvents();
void readSensor();
void updatePumpControl();
void updateDisplay();
void handleIoTCommands(const CommandData& cmd);
void handleIoTConfig(const String& configJson);
void sendTelemetry();

// ==================== SETUP ====================
void setup() {
    #if ENABLE_SERIAL_DEBUG
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println("\n\n=================================");
    Serial.println("Smart Water Pump System");
    Serial.println("Firmware Version: " FIRMWARE_VERSION);
    Serial.println("=================================\n");
    #endif
    
    // Initialize storage
    if (!storage.begin()) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("FATAL: Storage initialization failed!");
        #endif
        systemState = STATE_ERROR;
        return;
    }
    
    // Initialize hardware
    pumpController.begin();
    sensor.begin();
    displayManager.begin();
    buttonHandler.begin();

    // Initialize WiFi manager early (needed for TCP/IP stack even in simulation)
    // NOTE: Once TCP/IP stack is initialized, it keeps running even if:
    //       - WiFi disconnects later
    //       - AP mode fails to start
    //       - Network connection is lost
    // This prevents crashes in AsyncWebServer which depends on TCP/IP stack
    #if ENABLE_SERIAL_DEBUG
    #if SIMULATION_MODE
    Serial.println("Initializing WiFi (TCP/IP stack for web server)...");
    #endif
    #endif

    if (!wifiManager.begin()) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("WARNING: WiFi initialization failed!");
        Serial.println("Web server will not be available");
        Serial.println("System will continue in standalone mode");
        #endif
        wifiInitialized = false;
        // Don't crash - just continue without web server
    } else {
        delay(100); // Brief delay to ensure TCP/IP stack is ready
        wifiInitialized = true;
        #if ENABLE_SERIAL_DEBUG
        Serial.println("WiFi Manager initialized - TCP/IP stack ready");
        #endif
    }

    // ✅ Check if first-time setup is needed (for tank configuration)
    if (storage.isFirstTimeSetup()) {
        systemState = STATE_FIRST_TIME_SETUP;
        #if ENABLE_SERIAL_DEBUG
        Serial.println("First-time setup required");
        #endif
        // Set display to setup screen immediately to prevent flickering
        #if SIMULATION_MODE
        displayManager.showSetupScreen("SIMULATION\nAccess web UI\nfor setup");
        #else
        displayManager.showSetupScreen("Connect to WiFi:\n" + String(AP_SSID) + "\nPassword: " + String(AP_PASSWORD));
        #endif
    } else {
        systemState = STATE_NORMAL_OPERATION;
        displayManager.showMessage("System", "Initializing...", 2000);
        // Don't block - let loop handle initialization
    }
}

// ==================== MAIN LOOP ====================
void loop() {
    // Update all components
    buttonHandler.loop();
    displayManager.loop();
    
    // State machine
    switch (systemState) {
        case STATE_FIRST_TIME_SETUP:
            firstTimeSetup();
            break;
            
        case STATE_NORMAL_OPERATION:
            normalOperation();
            break;
            
        case STATE_CONFIG_MODE:
            configMode();
            break;
            
        case STATE_ERROR:
            displayManager.showError("System Error");
            delay(5000);
            ESP.restart();
            break;
    }
    
    // Handle button events across all states
    handleButtonEvents();
}

// ==================== INITIALIZATION ====================
void initializeSystem() {
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Initializing system...");
    #endif
    
    // Load configuration
    currentConfig = storage.loadTankConfig();
    calculator.setTankConfig(currentConfig);

    // WiFi manager already initialized in setup(), now configure it
    if (!currentConfig.firstTimeSetup) {
        displayManager.showMessage("WiFi", "Connecting...", 2000);

        #if IOT_ENABLED
        if (wifiManager.connectToSavedWiFi()) {
            displayManager.showMessage("WiFi", "Connected!", 2000);

            // Start mDNS
            wifiManager.startMDNS("waterpump");

            // Try to initialize IoT client (will fail gracefully if server unavailable)
            if (iotClient.begin()) {
                displayManager.showMessage("Cloud", "Connected!", 2000);

                // Set up callbacks
                iotClient.setCommandCallback(handleIoTCommands);
                iotClient.setConfigCallback(handleIoTConfig);

                // Initialize sync manager
                syncManager.begin(&storage, &iotClient);

                // Initial config sync (optional, continues if fails)
                syncManager.syncConfig();
            } else {
                #if ENABLE_SERIAL_DEBUG
                Serial.println("IoT connection failed - continuing in standalone mode");
                #endif
                displayManager.showMessage("System", "Standalone Mode", 2000);
            }
        } else {
            #if ENABLE_SERIAL_DEBUG
            Serial.println("WiFi connection failed - running standalone");
            #endif
            displayManager.showMessage("System", "No WiFi - OK", 2000);
        }
        #else
        displayManager.showMessage("System", "IoT Disabled", 2000);
        #endif
    }
    
    // Initialize water tracker
    waterTracker.begin(&storage, &calculator);
    
    // Try to start web server (optional - only if WiFi/TCP-IP available)
    if (wifiInitialized) {
        if (webServer.begin(&storage, &calculator, &pumpController, &waterTracker)) {
            displayManager.showMessage("WebServer", "Started!", 2000);
        } else {
            #if ENABLE_SERIAL_DEBUG
            Serial.println("Web server failed to start");
            #endif
        }
    } else {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Web server disabled - WiFi not initialized");
        #endif
    }
    
    // Try to initialize OTA updater (optional - only if WiFi available)
    if (otaUpdater.begin(&storage, IOT_SERVER_URL, currentConfig.deviceToken)) {
        displayManager.showMessage("OTA", "Ready", 2000);
        
        #if OTA_CHECK_AT_STARTUP
        if (otaUpdater.checkForUpdate()) {
            displayManager.showMessage("Update", "Available!", 2000);
        }
        #endif
    }
    
    // Try to initialize ML predictor (optional - works without model)
    if (mlPredictor.begin(&storage, IOT_SERVER_URL, currentConfig.deviceToken)) {
        if (mlPredictor.isReady()) {
            displayManager.showMessage("ML", "Model Loaded", 2000);
        } else {
            displayManager.showMessage("ML", "Using Fallback", 2000);
        }
    }
    
    // Initial sensor reading
    readSensor();
    
    systemInitialized = true;
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("System initialization complete");
    #endif
}

// ==================== FIRST TIME SETUP ====================
void firstTimeSetup() {
    // ✅ FIX: Static variables to ensure one-time initialization
    static bool networkStarted = false;
    static bool displayInitialized = false;

    // Start network (WiFi or AP) for configuration (only once)
    if (!networkStarted) {
        networkStarted = true;

        // Check if WiFi credentials exist
        String ssid, password;
        bool hasWiFiCreds = storage.loadWiFiCredentials(ssid, password);

        #if ENABLE_SERIAL_DEBUG
        Serial.println("=================================");
        Serial.println("FIRST TIME SETUP MODE");
        #endif

        if (hasWiFiCreds) {
            // WiFi credentials found - try to connect
            #if ENABLE_SERIAL_DEBUG
            Serial.print("WiFi credentials found - connecting to: ");
            Serial.println(ssid);
            #endif
            wifiManager.connectToWiFi(ssid, password);
        } else {
            // No WiFi credentials - start AP mode (real hardware) or use Wokwi-GUEST (simulation)
            #if SIMULATION_MODE
            #if ENABLE_SERIAL_DEBUG
            Serial.println("MODE: SIMULATION");
            Serial.println("No saved WiFi - will use Wokwi-GUEST");
            Serial.println("Access web interface via network");
            #endif
            #else
            // Real hardware - start AP mode
            #if ENABLE_SERIAL_DEBUG
            Serial.print("No WiFi credentials - starting AP: ");
            Serial.println(AP_SSID);
            Serial.print("Password: ");
            Serial.println(AP_PASSWORD);
            #endif
            if (!wifiManager.startAP()) {
                #if ENABLE_SERIAL_DEBUG
                Serial.println("ERROR: Failed to start Access Point!");
                Serial.println("System will continue in standalone mode");
                #endif
            } else {
                #if ENABLE_SERIAL_DEBUG
                Serial.print("Then open: http://");
                Serial.println(wifiManager.getAPIP());
                #endif
            }
            #endif
        }

        #if ENABLE_SERIAL_DEBUG
        Serial.println("=================================");
        #endif

        // Start web server for setup (only if WiFi/TCP-IP initialized)
        if (!webServer.isRunning() && wifiInitialized) {
            if (!webServer.begin(&storage, &calculator, &pumpController, &waterTracker)) {
                #if ENABLE_SERIAL_DEBUG
                Serial.println("WARNING: Web server failed to start!");
                Serial.println("Check WiFi/network connectivity");
                #endif
            }
        } else if (!wifiInitialized) {
            #if ENABLE_SERIAL_DEBUG
            Serial.println("Web server skipped - WiFi not initialized");
            #endif
        }
    }

    // ✅ FIX: Show setup screen immediately on first call, then keep it displayed
    if (!displayInitialized) {
        displayInitialized = true;
        #if SIMULATION_MODE
        displayManager.showSetupScreen("SIMULATION\nAccess web UI\nfor setup");
        #else
        displayManager.showSetupScreen("Connect to WiFi:\n" + String(AP_SSID) + "\nPassword: " + String(AP_PASSWORD));
        #endif
    }
    
    // ✅ FIX: Check setup status only periodically (every 2 seconds)
    static unsigned long lastSetupCheck = 0;
    if (millis() - lastSetupCheck > 2000) {
        lastSetupCheck = millis();
        
        // Check if setup is complete
        if (!storage.isFirstTimeSetup()) {
            #if ENABLE_SERIAL_DEBUG
            Serial.println("Setup completed! Transitioning to normal operation...");
            #endif

            // Switch back to main screen and show completion message
            displayManager.setScreen(SCREEN_MAIN);
            displayManager.showMessage("Setup", "Complete!", 2000);

            systemState = STATE_NORMAL_OPERATION;
            // initializeSystem() will be called by normalOperation() on next loop
        }
    }
}

// ==================== NORMAL OPERATION ====================
void normalOperation() {
    // Initialize system on first run
    if (!systemInitialized) {
        initializeSystem();
        return; // Skip rest of loop during initialization
    }

    // Read sensor periodically
    if (millis() - lastSensorRead > SENSOR_SAMPLE_INTERVAL_MS) {
        readSensor();
    }
    
    // Update pump control
    updatePumpControl();
    
    // Update display
    updateDisplay();
    
    // Update IoT (OPTIONAL - only if connected)
    if (wifiManager.isConnected()) {
        wifiManager.loop();
        
        #if IOT_ENABLED
        if (iotClient.isConnected()) {
            iotClient.loop();
            syncManager.loop();
            
            // Send telemetry (optional)
            if (millis() - lastTelemetrySend > TELEMETRY_SEND_INTERVAL_MS) {
                sendTelemetry();
            }
        }
        #endif
    }
    
    // Update water tracker
    waterTracker.loop();
    waterTracker.updateState(currentWaterLevel, pumpController.isOn(), currentInflow);
    
    // Update pump controller
    pumpController.loop();
    
    // Update optional features (gracefully skip if not available)
    if (webServer.isRunning()) {
        webServer.updateData(currentWaterLevel, currentInflow, maxInflow);
    }
    
    if (otaUpdater.isAutoUpdateEnabled()) {
        otaUpdater.loop();
    }
    
    if (mlPredictor.isEnabled()) {
        mlPredictor.loop();
    }
}

// ==================== CONFIGURATION MODE ====================
void configMode() {
    static int selectedItem = 0;
    
    displayManager.showConfigMenu(selectedItem);
    
    // Handle config menu navigation
    ButtonEvent event = buttonHandler.getEvent();
    
    if (event == BTN_TOP_PRESS) {
        selectedItem = (selectedItem - 1 + 6) % 6;
    } else if (event == BTN_BOTTOM_PRESS) {
        selectedItem = (selectedItem + 1) % 6;
    } else if (event == BTN_MID_PRESS) {
        // Handle menu selection
        switch (selectedItem) {
            case 0: // Tank Height
                // TODO: Implement tank height configuration
                break;
            case 1: // Tank Dimensions
                // TODO: Implement dimensions configuration
                break;
            case 2: // Thresholds
                // TODO: Implement thresholds configuration
                break;
            case 3: // WiFi Setup
                // TODO: Implement WiFi setup
                break;
            case 4: // Factory Reset
                displayManager.showMessage("Reset", "Hold MID 5s", 3000);
                break;
            case 5: // Exit
                systemState = STATE_NORMAL_OPERATION;
                break;
        }
    } else if (event == BTN_MID_LONG_PRESS && selectedItem == 4) {
        // Factory reset
        displayManager.showMessage("Reset", "Resetting...", 3000);
        storage.factoryReset();
        delay(1000); // Brief delay before restart
        ESP.restart();
    }
}

// ==================== BUTTON EVENT HANDLING ====================
void handleButtonEvents() {
    ButtonEvent event = buttonHandler.getEvent();

    if (event == BTN_NONE) return;

    // Don't allow screen switching during first-time setup
    if (systemState == STATE_FIRST_TIME_SETUP) {
        return;
    }

    switch (event) {
        case BTN_LEFT_PRESS:
            displayManager.previousScreen();
            break;

        case BTN_RIGHT_PRESS:
            displayManager.nextScreen();
            break;
            
        case BTN_MID_PRESS:
            if (systemState == STATE_NORMAL_OPERATION) {
                systemState = STATE_CONFIG_MODE;
            }
            break;
            
        case BTN_MANUAL_SWITCH_TOGGLE:
            if (pumpController.getMode() == MANUAL_MODE || pumpController.getMode() == OVERRIDE_MODE) {
                pumpController.toggleManual();
            } else {
                // Switch to manual mode
                pumpController.setMode(MANUAL_MODE);
                pumpController.toggleManual();
            }
            break;
            
        case BTN_MANUAL_SWITCH_LONG_PRESS:
            // Enter/exit override mode
            if (pumpController.getMode() == OVERRIDE_MODE) {
                pumpController.exitOverrideMode();
                displayManager.showMessage("Mode", "AUTO Mode", 2000);
            } else {
                pumpController.enterOverrideMode();
                displayManager.showMessage("Mode", "OVERRIDE!", 2000);
            }
            break;
            
        default:
            break;
    }
}

// ==================== SENSOR READING ====================
void readSensor() {
    lastSensorRead = millis();
    
    float distance = sensor.getAverageDistance(3);
    
    if (distance < 0) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Sensor read error");
        #endif
        return;
    }
    
    // Update water level
    previousWaterLevel = currentWaterLevel;
    currentWaterLevel = calculator.distanceToLevel(distance);
    
    // Calculate inflow
    unsigned long deltaTime = millis() - lastSensorRead;
    currentInflow = calculator.calculateInflow(currentWaterLevel, previousWaterLevel, deltaTime);
    
    // Update max inflow
    if (currentInflow > maxInflow) {
        maxInflow = currentInflow;
        currentConfig.maxInflow = maxInflow;
        storage.saveTankConfig(currentConfig);
    }
    
    #if ENABLE_SERIAL_DEBUG
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.print(" cm, Level: ");
    Serial.print(currentWaterLevel);
    Serial.print(" %, Inflow: ");
    Serial.println(currentInflow);
    #endif
}

// ==================== PUMP CONTROL ====================
void updatePumpControl() {
    // Safety checks
    pumpController.updateSafetyCheck(currentWaterLevel, previousWaterLevel, 
                                     SENSOR_SAMPLE_INTERVAL_MS);
    
    // Automatic control if in AUTO mode
    if (pumpController.getMode() == AUTO_MODE) {
        pumpController.autoControl(currentWaterLevel, 
                                   currentConfig.upperThreshold, 
                                   currentConfig.lowerThreshold);
    }
    
    // Save pump cycle data
    if (pumpController.getLastStateChangeTime() > 0) {
        PumpCycle cycle;
        cycle.timestamp = millis();
        cycle.motorState = pumpController.isOn();
        cycle.waterLevel = currentWaterLevel;
        cycle.inflow = currentInflow;
        storage.savePumpCycle(cycle);
    }
}

// ==================== DISPLAY UPDATE ====================
void updateDisplay() {
    DisplayData data;
    data.waterLevel = currentWaterLevel;
    data.currentInflow = currentInflow;
    data.maxInflow = maxInflow;
    data.motorState = pumpController.isOn();
    data.manualMode = (pumpController.getMode() == MANUAL_MODE);
    data.overrideMode = (pumpController.getMode() == OVERRIDE_MODE);
    data.dailyUsage = waterTracker.getTodayUsage();
    data.monthlyUsage = waterTracker.getMonthUsage();
    data.wifiStatus = wifiManager.isConnected() ? "Connected" : "Disconnected";
    data.iotStatus = iotClient.isConnected() ? "Online" : "Offline";
    data.dryRunAlarm = pumpController.isDryRunDetected();
    data.overflowAlarm = pumpController.isOverflowRisk();
    
    displayManager.updateData(data);
}

// ==================== IOT COMMAND HANDLING ====================
void handleIoTCommands(const CommandData& cmd) {
    #if ENABLE_SERIAL_DEBUG
    Serial.print("Received IoT command: ");
    Serial.println(cmd.command);
    #endif
    
    if (cmd.command == "pump_on") {
        pumpController.turnOn();
    } else if (cmd.command == "pump_off") {
        pumpController.turnOff();
    } else if (cmd.command == "set_mode_auto") {
        pumpController.setMode(AUTO_MODE);
    } else if (cmd.command == "set_mode_manual") {
        pumpController.setMode(MANUAL_MODE);
    } else if (cmd.command == "update_config") {
        handleIoTConfig(cmd.payload);
    } else if (cmd.command == "reset_safety") {
        pumpController.resetSafetyAlarms();
    } else if (cmd.command == "restart") {
        displayManager.showMessage("System", "Restarting...", 2000);
        delay(2000);
        ESP.restart();
    }
}

// ==================== IOT CONFIG HANDLING ====================
void handleIoTConfig(const String& configJson) {
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Received config update from cloud");
    #endif
    
    syncManager.onCloudConfigReceived(configJson);
    
    // Reload config
    currentConfig = storage.loadTankConfig();
    calculator.setTankConfig(currentConfig);
}

// ==================== TELEMETRY SENDING ====================
void sendTelemetry() {
    if (!iotClient.isConnected()) return;
    
    TelemetryData telemetry;
    telemetry.timestamp = millis();
    telemetry.motorState = pumpController.isOn();
    telemetry.waterLevel = currentWaterLevel;
    telemetry.currentInflow = currentInflow;
    telemetry.maxInflow = maxInflow;
    telemetry.dailyUsage = waterTracker.getTodayUsage();
    telemetry.monthlyUsage = waterTracker.getMonthUsage();
    
    if (iotClient.sendTelemetry(telemetry)) {
        lastTelemetrySend = millis();
    }
}