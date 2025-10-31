// storage_manager.cpp
#include "storage_manager.h"
#include "config.h"

StorageManager::StorageManager() {}

bool StorageManager::begin() {
    return preferences.begin(NAMESPACE, false);
}

bool StorageManager::saveTankConfig(const TankConfig& config) {
    if (!preferences.begin(NAMESPACE, false)) return false;
    
    preferences.putBool("firstSetup", config.firstTimeSetup);
    preferences.putFloat("tankHeight", config.tankHeight);
    preferences.putFloat("tankLength", config.tankLength);
    preferences.putFloat("tankWidth", config.tankWidth);
    preferences.putFloat("tankRadius", config.tankRadius);
    preferences.putUChar("tankShape", (uint8_t)config.shape);
    preferences.putFloat("upperThresh", config.upperThreshold);
    preferences.putFloat("lowerThresh", config.lowerThreshold);
    preferences.putFloat("maxInflow", config.maxInflow);
    preferences.putString("devToken", config.deviceToken);
    preferences.putULong("configVer", config.configVersion);
    preferences.putString("modSource", config.lastModifiedSource);
    preferences.putBool("needsSync", config.needsSync);
    preferences.putUChar("syncMode", (uint8_t)config.syncMode);
    
    preferences.end();
    return true;
}

TankConfig StorageManager::loadTankConfig() {
    TankConfig config;
    
    if (!preferences.begin(NAMESPACE, true)) return config;
    
    config.firstTimeSetup = preferences.getBool("firstSetup", true);
    config.tankHeight = preferences.getFloat("tankHeight", 0.0);
    config.tankLength = preferences.getFloat("tankLength", 0.0);
    config.tankWidth = preferences.getFloat("tankWidth", 0.0);
    config.tankRadius = preferences.getFloat("tankRadius", 0.0);
    config.shape = (TankShape)preferences.getUChar("tankShape", 0);
    config.upperThreshold = preferences.getFloat("upperThresh", DEFAULT_UPPER_THRESHOLD);
    config.lowerThreshold = preferences.getFloat("lowerThresh", DEFAULT_LOWER_THRESHOLD);
    config.maxInflow = preferences.getFloat("maxInflow", 0.0);
    config.deviceToken = preferences.getString("devToken", "");
    config.configVersion = preferences.getULong("configVer", 0);
    config.lastModifiedSource = preferences.getString("modSource", "device");
    config.needsSync = preferences.getBool("needsSync", false);
    config.syncMode = (SyncMode)preferences.getUChar("syncMode", 0);
    
    preferences.end();
    return config;
}

bool StorageManager::isFirstTimeSetup() {
    preferences.begin(NAMESPACE, true);
    bool firstSetup = preferences.getBool("firstSetup", true);
    preferences.end();
    return firstSetup;
}

void StorageManager::markSetupComplete() {
    preferences.begin(NAMESPACE, false);
    preferences.putBool("firstSetup", false);
    preferences.end();
}

bool StorageManager::saveWiFiCredentials(const String& ssid, const String& password) {
    preferences.begin(NAMESPACE, false);
    preferences.putString("wifiSSID", ssid);
    preferences.putString("wifiPass", password);
    preferences.end();
    return true;
}

bool StorageManager::loadWiFiCredentials(String& ssid, String& password) {
    preferences.begin(NAMESPACE, true);
    ssid = preferences.getString("wifiSSID", "");
    password = preferences.getString("wifiPass", "");
    preferences.end();

    // If no credentials found and in simulation mode, use Wokwi defaults
    #if SIMULATION_MODE
    if (ssid.isEmpty()) {
        ssid = "Wokwi-GUEST";
        password = "";
        #if ENABLE_SERIAL_DEBUG
        Serial.println("No saved WiFi - using Wokwi-GUEST defaults");
        #endif
        return true; // Return true so it attempts connection
    }
    #endif

    return !ssid.isEmpty();
}

bool StorageManager::saveDeviceToken(const String& token) {
    preferences.begin(NAMESPACE, false);
    preferences.putString("devToken", token);
    preferences.end();
    return true;
}

String StorageManager::loadDeviceToken() {
    preferences.begin(NAMESPACE, true);
    String token = preferences.getString("devToken", "");
    preferences.end();
    return token;
}

bool StorageManager::updateConfigVersion(const String& source) {
    preferences.begin(NAMESPACE, false);
    unsigned long version = preferences.getULong("configVer", 0) + 1;
    preferences.putULong("configVer", version);
    preferences.putString("modSource", source);
    preferences.end();
    return true;
}

bool StorageManager::markNeedsSync(bool needs) {
    preferences.begin(NAMESPACE, false);
    preferences.putBool("needsSync", needs);
    preferences.end();
    return true;
}

bool StorageManager::setSyncMode(SyncMode mode) {
    preferences.begin(NAMESPACE, false);
    preferences.putUChar("syncMode", (uint8_t)mode);
    preferences.end();
    return true;
}

bool StorageManager::savePumpCycle(const PumpCycle& cycle) {
    int currentIndex = getCurrentCycleIndex();
    String key = generateCycleKey(currentIndex);
    
    preferences.begin(NAMESPACE, false);
    
    // Create JSON string to store cycle data
    JsonDocument doc;
    doc["ts"] = cycle.timestamp;
    doc["state"] = cycle.motorState;
    doc["level"] = cycle.waterLevel;
    doc["inflow"] = cycle.inflow;
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    preferences.putString(key.c_str(), jsonStr);
    
    incrementCycleIndex();
    preferences.end();
    return true;
}

bool StorageManager::getPumpCycles(PumpCycle* cycles, int maxCount, int& actualCount) {
    preferences.begin(NAMESPACE, true);
    int currentIndex = preferences.getInt("cycleIdx", 0);
    
    actualCount = 0;
    for (int i = 0; i < min(maxCount, MAX_PUMP_CYCLE_LOGS); i++) {
        int idx = (currentIndex - 1 - i + MAX_PUMP_CYCLE_LOGS) % MAX_PUMP_CYCLE_LOGS;
        String key = generateCycleKey(idx);
        String jsonStr = preferences.getString(key.c_str(), "");
        
        if (jsonStr.isEmpty()) continue;
        
        JsonDocument doc;
        deserializeJson(doc, jsonStr);
        
        cycles[actualCount].timestamp = doc["ts"];
        cycles[actualCount].motorState = doc["state"];
        cycles[actualCount].waterLevel = doc["level"];
        cycles[actualCount].inflow = doc["inflow"];
        actualCount++;
    }
    
    preferences.end();
    return actualCount > 0;
}

bool StorageManager::saveDailyUsage(const DailyUsage& usage) {
    String key = generateDailyKey(usage.date);
    
    preferences.begin(NAMESPACE, false);
    JsonDocument doc;
    doc["date"] = usage.date;
    doc["usage"] = usage.totalUsageLiters;
    doc["cycles"] = usage.pumpCycles;
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    preferences.putString(key.c_str(), jsonStr);
    preferences.end();
    return true;
}

bool StorageManager::getDailyUsage(unsigned long date, DailyUsage& usage) {
    String key = generateDailyKey(date);
    
    preferences.begin(NAMESPACE, true);
    String jsonStr = preferences.getString(key.c_str(), "");
    preferences.end();
    
    if (jsonStr.isEmpty()) return false;
    
    JsonDocument doc;
    deserializeJson(doc, jsonStr);
    
    usage.date = doc["date"];
    usage.totalUsageLiters = doc["usage"];
    usage.pumpCycles = doc["cycles"];
    
    return true;
}

bool StorageManager::getMonthlyUsage(int year, int month, float& totalLiters) {
    // Calculate timestamps for first and last day of month
    struct tm timeinfo = {0};
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = 1;
    unsigned long startDate = mktime(&timeinfo);
    
    // Get days in month
    timeinfo.tm_mon = month;
    timeinfo.tm_mday = 0;
    int daysInMonth = mktime(&timeinfo) / 86400 - startDate / 86400;
    
    totalLiters = 0.0;
    for (int day = 0; day < daysInMonth; day++) {
        DailyUsage usage;
        if (getDailyUsage(startDate + day * 86400, usage)) {
            totalLiters += usage.totalUsageLiters;
        }
    }
    
    return true;
}

bool StorageManager::getLast30DaysUsage(DailyUsage* usageArray, int& count) {
    unsigned long currentTime = time(nullptr);
    count = 0;
    
    for (int i = 0; i < 30; i++) {
        unsigned long date = currentTime - (i * 86400);
        DailyUsage usage;
        if (getDailyUsage(date, usage)) {
            usageArray[count++] = usage;
        }
    }
    
    return count > 0;
}

bool StorageManager::saveWebCredentials(const String& username, const String& password) {
    preferences.begin(NAMESPACE, false);
    preferences.putString("webUser", username);
    preferences.putString("webPass", password);
    preferences.end();
    return true;
}

bool StorageManager::loadWebCredentials(String& username, String& password) {
    preferences.begin(NAMESPACE, true);
    username = preferences.getString("webUser", WEB_DEFAULT_USERNAME);
    password = preferences.getString("webPass", WEB_DEFAULT_PASSWORD);
    preferences.end();
    return true;
}

bool StorageManager::saveOTAEnabled(bool enabled) {
    preferences.begin(NAMESPACE, false);
    preferences.putBool("otaEnabled", enabled);
    preferences.end();
    return true;
}

bool StorageManager::isOTAEnabled() {
    preferences.begin(NAMESPACE, true);
    bool enabled = preferences.getBool("otaEnabled", AUTO_OTA_ENABLED);
    preferences.end();
    return enabled;
}

bool StorageManager::saveMLModelTimestamp(unsigned long timestamp) {
    preferences.begin(NAMESPACE, false);
    preferences.putULong("mlTimestamp", timestamp);
    preferences.end();
    return true;
}

unsigned long StorageManager::getMLModelTimestamp() {
    preferences.begin(NAMESPACE, true);
    unsigned long timestamp = preferences.getULong("mlTimestamp", 0);
    preferences.end();
    return timestamp;
}

void StorageManager::factoryReset() {
    preferences.begin(NAMESPACE, false);
    preferences.clear();
    preferences.end();
}

// Private helper functions
String StorageManager::generateCycleKey(int index) {
    return "cycle" + String(index);
}

String StorageManager::generateDailyKey(unsigned long date) {
    // Normalize to midnight
    unsigned long midnightDate = (date / 86400) * 86400;
    return "day" + String(midnightDate);
}

int StorageManager::getCurrentCycleIndex() {
    preferences.begin(NAMESPACE, true);
    int idx = preferences.getInt("cycleIdx", 0);
    preferences.end();
    return idx;
}

void StorageManager::incrementCycleIndex() {
    preferences.begin(NAMESPACE, false);
    int idx = preferences.getInt("cycleIdx", 0);
    idx = (idx + 1) % MAX_PUMP_CYCLE_LOGS;
    preferences.putInt("cycleIdx", idx);
    preferences.end();
}