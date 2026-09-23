// main.cpp - Smart Water Pump System with ML Integration
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

// Global objects
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

// Global state
TankConfig currentConfig;
float currentWaterLevel = 0.0;
float previousWaterLevel = 0.0;
float currentInflow = 0.0;
float maxInflow = 0.0;
unsigned long lastSensorRead = 0;
unsigned long lastTelemetrySend = 0;
unsigned long lastMLPrediction = 0;
bool systemInitialized = false;
bool wifiInitialized = false;
bool iotAvailable = false;
bool mlModelAvailable = false;

enum SystemState {
    STATE_FIRST_TIME_SETUP,
    STATE_NORMAL_OPERATION,
    STATE_CONFIG_MODE,
    STATE_ERROR
};

SystemState systemState = STATE_FIRST_TIME_SETUP;

// Function declarations
void initializeSystem();
void firstTimeSetup();
void normalOperation();
void configMode();
void handleButtonEvents();
void readSensor();
void updatePumpControl();
void updatePumpControlWithML();
void updateDisplay();
void handleIoTCommands(const CommandData& cmd);
void handleIoTConfig(const String& configJson);
void sendTelemetry();
void startNetworkForSetup();

void setup() {
    #if ENABLE_SERIAL_DEBUG
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println("\n\n=================================");
    Serial.println("Smart Water Pump System");
    Serial.println("Firmware Version: " FIRMWARE_VERSION);
    Serial.println("=================================\n");
    #endif
    
    if (!storage.begin()) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("FATAL: Storage initialization failed!");
        #endif
        systemState = STATE_ERROR;
        return;
    }
    
    pumpController.begin();
    sensor.begin();
    displayManager.begin();
    buttonHandler.begin();

    // Initialize WiFi/TCP stack
    if (!wifiManager.begin()) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("WARNING: WiFi initialization failed!");
        #endif
        wifiInitialized = false;
    } else {
        delay(100);
        wifiInitialized = true;
        #if ENABLE_SERIAL_DEBUG
        Serial.println("WiFi Manager initialized");
        #endif
    }

    // Check if first-time setup needed
    if (storage.isFirstTimeSetup()) {
        systemState = STATE_FIRST_TIME_SETUP;
        #if ENABLE_SERIAL_DEBUG
        Serial.println("First-time setup required");
        #endif
        #if SIMULATION_MODE
        displayManager.showSetupScreen("SIMULATION\nAccess web UI\nat localhost:8180");
        #else
        displayManager.showSetupScreen("Connect to WiFi:\n" + String(AP_SSID) + "\nPassword: " + String(AP_PASSWORD));
        #endif
    } else {
        systemState = STATE_NORMAL_OPERATION;
        displayManager.showMessage("System", "Initializing...", 2000);
    }
}

void loop() {
    buttonHandler.loop();
    displayManager.loop();
    
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
    
    handleButtonEvents();
}

void initializeSystem() {
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Initializing system...");
    #endif
    
    currentConfig = storage.loadTankConfig();
    calculator.setTankConfig(currentConfig);

    if (!currentConfig.firstTimeSetup) {
        displayManager.showMessage("WiFi", "Connecting...", 2000);

        // Try to connect to WiFi (optional, system works standalone)
        #if IOT_ENABLED
        if (wifiManager.connectToSavedWiFi()) {
            displayManager.showMessage("WiFi", "Connected!", 2000);
            wifiManager.startMDNS("waterpump");

            // Try to connect to IoT (optional, graceful failure)
            if (iotClient.begin()) {
                displayManager.showMessage("Cloud", "Connected!", 2000);
                iotClient.setCommandCallback(handleIoTCommands);
                iotClient.setConfigCallback(handleIoTConfig);
                syncManager.begin(&storage, &iotClient);
                syncManager.syncConfig();
                iotAvailable = true;
            } else {
                #if ENABLE_SERIAL_DEBUG
                Serial.println("IoT unavailable - standalone mode");
                #endif
                displayManager.showMessage("System", "Standalone Mode", 2000);
                iotAvailable = false;
            }
        } else {
            #if ENABLE_SERIAL_DEBUG
            Serial.println("WiFi failed - standalone mode");
            #endif
            displayManager.showMessage("System", "No WiFi - OK", 2000);
            iotAvailable = false;
        }
        #else
        displayManager.showMessage("System", "IoT Disabled", 2000);
        iotAvailable = false;
        #endif
    }
    
    waterTracker.begin(&storage, &calculator);
    
    // Try to start web server (optional)
    if (wifiInitialized) {
        if (webServer.begin(&storage, &calculator, &pumpController, &waterTracker)) {
            #if ENABLE_SERIAL_DEBUG
            Serial.println("Web server started");
            #if SIMULATION_MODE
            Serial.println("Access at: http://localhost:8180");
            #endif
            #endif
            displayManager.showMessage("WebServer", "Started!", 2000);
        }
    }
    
    // Try to initialize OTA (optional)
    if (iotAvailable && otaUpdater.begin(&storage, IOT_SERVER_URL, currentConfig.deviceToken)) {
        displayManager.showMessage("OTA", "Ready", 2000);
        
        #if OTA_CHECK_AT_STARTUP
        if (otaUpdater.checkForUpdate()) {
            displayManager.showMessage("Update", "Available!", 2000);
        }
        #endif
    }
    
    // Try to initialize ML predictor (optional, works without model)
    #if ML_MODEL_ENABLED
    if (iotAvailable && mlPredictor.begin(&storage, IOT_SERVER_URL, currentConfig.deviceToken)) {
        if (mlPredictor.isReady()) {
            displayManager.showMessage("ML", "Model Loaded", 2000);
            mlModelAvailable = true;
        } else {
            displayManager.showMessage("ML", "Fallback Mode", 2000);
            mlModelAvailable = false;
        }
    } else {
        mlModelAvailable = false;
    }
    #else
    mlModelAvailable = false;
    #endif
    
    readSensor();
    systemInitialized = true;
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("System initialization complete");
    Serial.println("Status:");
    Serial.print("  - WiFi: ");
    Serial.println(wifiInitialized ? "Available" : "Unavailable");
    Serial.print("  - IoT: ");
    Serial.println(iotAvailable ? "Connected" : "Unavailable");
    Serial.print("  - ML Model: ");
    Serial.println(mlModelAvailable ? "Loaded" : "Unavailable");
    Serial.println("System is fully operational in standalone mode");
    #endif
}

void firstTimeSetup() {
    static bool networkStarted = false;
    static unsigned long lastSetupCheck = 0;

    if (!networkStarted) {
        networkStarted = true;
        startNetworkForSetup();
    }

    // Button-based setup wizard
    TankConfig setupConfig = storage.loadTankConfig();
    ButtonEvent event = buttonHandler.getEvent();
    
    if (displayManager.handleSetupWizard(event, setupConfig)) {
        setupConfig.firstTimeSetup = false;
        storage.saveTankConfig(setupConfig);
        storage.markSetupComplete();
        
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Setup completed via buttons!");
        #endif
        
        displayManager.showMessage("Setup", "Complete!", 2000);
        systemState = STATE_NORMAL_OPERATION;
        displayManager.resetSetupWizard();
    }

    // Check if setup completed via web
    if (millis() - lastSetupCheck > 2000) {
        lastSetupCheck = millis();
        
        if (!storage.isFirstTimeSetup()) {
            #if ENABLE_SERIAL_DEBUG
            Serial.println("Setup completed via web!");
            #endif
            
            displayManager.setScreen(SCREEN_MAIN);
            displayManager.showMessage("Setup", "Complete!", 2000);
            systemState = STATE_NORMAL_OPERATION;
            displayManager.resetSetupWizard();
        }
    }
}

void startNetworkForSetup() {
    #if ENABLE_SERIAL_DEBUG
    Serial.println("=================================");
    Serial.println("FIRST TIME SETUP MODE");
    #endif

    String ssid, password;
    bool hasWiFiCreds = storage.loadWiFiCredentials(ssid, password);

    #if SIMULATION_MODE
    if (!hasWiFiCreds || ssid.isEmpty()) {
        ssid = "Wokwi-GUEST";
        password = "";
        #if ENABLE_SERIAL_DEBUG
        Serial.println("MODE: SIMULATION - using Wokwi-GUEST");
        #endif
    }
    wifiManager.connectToWiFi(ssid, password);
    #else
    if (hasWiFiCreds && !ssid.isEmpty()) {
        #if ENABLE_SERIAL_DEBUG
        Serial.print("Connecting to saved WiFi: ");
        Serial.println(ssid);
        #endif
        wifiManager.connectToWiFi(ssid, password);
    } else {
        #if ENABLE_SERIAL_DEBUG
        Serial.print("Starting AP: ");
        Serial.println(AP_SSID);
        #endif
        wifiManager.startAP();
    }
    #endif

    #if ENABLE_SERIAL_DEBUG
    Serial.println("=================================");
    #endif

    if (!webServer.isRunning() && wifiInitialized) {
        webServer.begin(&storage, &calculator, &pumpController, &waterTracker);
    }
}

void normalOperation() {
    if (!systemInitialized) {
        initializeSystem();
        return;
    }

    // Read sensor periodically
    if (millis() - lastSensorRead > SENSOR_SAMPLE_INTERVAL_MS) {
        readSensor();
    }
    
    // Update pump control (with or without ML)
    if (mlModelAvailable && mlPredictor.isEnabled()) {
        updatePumpControlWithML();
    } else {
        updatePumpControl();
    }
    
    updateDisplay();
    
    // Optional: WiFi and IoT updates
    if (wifiManager.isConnected()) {
        wifiManager.loop();
        
        #if IOT_ENABLED
        if (iotAvailable && iotClient.isConnected()) {
            iotClient.loop();
            syncManager.loop();
            
            if (millis() - lastTelemetrySend > TELEMETRY_SEND_INTERVAL_MS) {
                sendTelemetry();
            }
        }
        #endif
    }
    
    // Core functionality (always works)
    waterTracker.loop();
    waterTracker.updateState(currentWaterLevel, pumpController.isOn(), currentInflow);
    pumpController.loop();
    
    // Optional features
    if (webServer.isRunning()) {
        webServer.updateData(currentWaterLevel, currentInflow, maxInflow);
    }
    
    #if AUTO_OTA_ENABLED
    if (iotAvailable && otaUpdater.isAutoUpdateEnabled()) {
        otaUpdater.loop();
    }
    #endif
    
    #if ML_MODEL_ENABLED
    if (iotAvailable && mlPredictor.isEnabled()) {
        mlPredictor.loop();
    }
    #endif
}

void configMode() {
    static int selectedItem = 0;
    
    displayManager.showConfigMenu(selectedItem);
    ButtonEvent event = buttonHandler.getEvent();
    
    if (event == BTN_TOP_PRESS) {
        selectedItem = (selectedItem - 1 + 6) % 6;
    } else if (event == BTN_BOTTOM_PRESS) {
        selectedItem = (selectedItem + 1) % 6;
    } else if (event == BTN_MID_PRESS) {
        switch (selectedItem) {
            case 4: // Factory Reset
                displayManager.showMessage("Reset", "Hold MID 5s", 3000);
                break;
            case 5: // Exit
                systemState = STATE_NORMAL_OPERATION;
                break;
        }
    } else if (event == BTN_MID_LONG_PRESS && selectedItem == 4) {
        displayManager.showMessage("Reset", "Resetting...", 3000);
        storage.factoryReset();
        delay(1000);
        ESP.restart();
    }
}

void handleButtonEvents() {
    ButtonEvent event = buttonHandler.getEvent();

    if (event == BTN_NONE) return;

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
                pumpController.setMode(MANUAL_MODE);
                pumpController.toggleManual();
            }
            break;
            
        case BTN_MANUAL_SWITCH_LONG_PRESS:
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

void readSensor() {
    lastSensorRead = millis();
    
    float distance = sensor.getAverageDistance(3);
    
    if (distance < 0) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Sensor read error");
        #endif
        return;
    }
    
    previousWaterLevel = currentWaterLevel;
    currentWaterLevel = calculator.distanceToLevel(distance);
    
    unsigned long deltaTime = millis() - lastSensorRead;
    currentInflow = calculator.calculateInflow(currentWaterLevel, previousWaterLevel, deltaTime);
    
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

void updatePumpControl() {
    // Safety checks (always active)
    pumpController.updateSafetyCheck(currentWaterLevel, previousWaterLevel, SENSOR_SAMPLE_INTERVAL_MS);
    
    // Automatic control if in AUTO mode
    if (pumpController.getMode() == AUTO_MODE) {
        pumpController.autoControl(currentWaterLevel, currentConfig.upperThreshold, currentConfig.lowerThreshold);
    }
    
    // Log pump cycles
    if (pumpController.getLastStateChangeTime() > 0) {
        PumpCycle cycle;
        cycle.timestamp = millis();
        cycle.motorState = pumpController.isOn();
        cycle.waterLevel = currentWaterLevel;
        cycle.inflow = currentInflow;
        storage.savePumpCycle(cycle);
    }
}

void updatePumpControlWithML() {
    // Safety checks (always active)
    pumpController.updateSafetyCheck(currentWaterLevel, previousWaterLevel, SENSOR_SAMPLE_INTERVAL_MS);
    
    // Use ML prediction if in AUTO mode
    if (pumpController.getMode() == AUTO_MODE && millis() - lastMLPrediction > ML_PREDICTION_INTERVAL_MS) {
        lastMLPrediction = millis();
        
        // Prepare ML input
        MLInput mlInput;
        int hour, minute, second;
        TimeUtils::getCurrentTime(hour, minute, second);
        mlInput.hourOfDay = hour;
        mlInput.dayOfWeek = TimeUtils::getDayOfWeek();
        mlInput.currentLevel = currentWaterLevel;
        mlInput.recentUsageRate = waterTracker.getTodayUsage() / (hour + 1); // L/hour
        mlInput.timeSinceLastFill = (millis() - pumpController.getLastStateChangeTime()) / 60000; // minutes
        mlInput.avgUsageSameHour = 0; // TODO: Calculate from historical data
        mlInput.isWeekend = (mlInput.dayOfWeek == 0 || mlInput.dayOfWeek == 6);
        
        // Get prediction
        MLPrediction prediction = mlPredictor.predict(mlInput);
        
        #if ENABLE_SERIAL_DEBUG
        Serial.print("ML Prediction - Should turn on: ");
        Serial.print(prediction.shouldTurnOn ? "YES" : "NO");
        Serial.print(", Confidence: ");
        Serial.println(prediction.confidence);
        #endif
        
        // Act on prediction with confidence threshold
        if (prediction.confidence > 0.7) { // Only act if confident
            if (prediction.shouldTurnOn && !pumpController.isOn()) {
                pumpController.turnOn();
            }
        } else {
            // Fallback to threshold-based control if low confidence
            pumpController.autoControl(currentWaterLevel, currentConfig.upperThreshold, currentConfig.lowerThreshold);
        }
    } else if (pumpController.getMode() == AUTO_MODE) {
        // Between predictions, use threshold-based control
        pumpController.autoControl(currentWaterLevel, currentConfig.upperThreshold, currentConfig.lowerThreshold);
    }
    
    // Log pump cycles
    if (pumpController.getLastStateChangeTime() > 0) {
        PumpCycle cycle;
        cycle.timestamp = millis();
        cycle.motorState = pumpController.isOn();
        cycle.waterLevel = currentWaterLevel;
        cycle.inflow = currentInflow;
        storage.savePumpCycle(cycle);
    }
}

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
    data.iotStatus = (iotAvailable && iotClient.isConnected()) ? "Online" : "Offline";
    data.dryRunAlarm = pumpController.isDryRunDetected();
    data.overflowAlarm = pumpController.isOverflowRisk();
    
    displayManager.updateData(data);
}

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
    } else if (cmd.command == "download_ml_model") {
        #if ML_MODEL_ENABLED
        if (mlPredictor.downloadModel()) {
            displayManager.showMessage("ML", "Model Updated", 2000);
            mlModelAvailable = mlPredictor.isReady();
        }
        #endif
    } else if (cmd.command == "restart") {
        displayManager.showMessage("System", "Restarting...", 2000);
        delay(2000);
        ESP.restart();
    }
}

void handleIoTConfig(const String& configJson) {
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Received config update from cloud");
    #endif
    
    syncManager.onCloudConfigReceived(configJson);
    currentConfig = storage.loadTankConfig();
    calculator.setTankConfig(currentConfig);
}

void sendTelemetry() {
    if (!iotAvailable || !iotClient.isConnected()) return;
    
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
