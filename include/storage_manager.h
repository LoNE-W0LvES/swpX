// storage_manager.h
#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <Preferences.h>
#include <ArduinoJson.h>

enum TankShape {
    RECTANGULAR,
    CYLINDRICAL
};

enum SyncMode {
    DEVICE_PRIORITY,
    CLOUD_PRIORITY
};

struct TankConfig {
    bool firstTimeSetup = true;
    float tankHeight = 0.0;       // cm
    float tankLength = 0.0;       // cm (for rectangular)
    float tankWidth = 0.0;        // cm (for rectangular)
    float tankRadius = 0.0;       // cm (for cylindrical)
    TankShape shape = RECTANGULAR;
    float upperThreshold = 100.0; // percentage
    float lowerThreshold = 20.0;  // percentage
    float maxInflow = 0.0;        // cm³/sec
    String deviceToken = "";
    unsigned long configVersion = 0;
    String lastModifiedSource = "device";
    bool needsSync = false;
    SyncMode syncMode = DEVICE_PRIORITY;
};

struct PumpCycle {
    unsigned long timestamp;
    bool motorState;
    float waterLevel;
    float inflow;
};

struct DailyUsage {
    unsigned long date;           // Unix timestamp (midnight)
    float totalUsageLiters;
    int pumpCycles;
};

class StorageManager {
public:
    StorageManager();
    bool begin();
    
    // Tank Configuration
    bool saveTankConfig(const TankConfig& config);
    TankConfig loadTankConfig();
    bool isFirstTimeSetup();
    void markSetupComplete();
    
    // WiFi Credentials
    bool saveWiFiCredentials(const String& ssid, const String& password);
    bool loadWiFiCredentials(String& ssid, String& password);
    
    // Device Token
    bool saveDeviceToken(const String& token);
    String loadDeviceToken();
    
    // Config Sync
    bool updateConfigVersion(const String& source);
    bool markNeedsSync(bool needs);
    bool setSyncMode(SyncMode mode);
    
    // Pump Cycle Logs
    bool savePumpCycle(const PumpCycle& cycle);
    bool getPumpCycles(PumpCycle* cycles, int maxCount, int& actualCount);
    
    // Daily Usage
    bool saveDailyUsage(const DailyUsage& usage);
    bool getDailyUsage(unsigned long date, DailyUsage& usage);
    bool getMonthlyUsage(int year, int month, float& totalLiters);
    bool getLast30DaysUsage(DailyUsage* usageArray, int& count);
    
    // Web Authentication
    bool saveWebCredentials(const String& username, const String& password);
    bool loadWebCredentials(String& username, String& password);
    
    // OTA Settings
    bool saveOTAEnabled(bool enabled);
    bool isOTAEnabled();
    
    // ML Model Settings
    bool saveMLModelTimestamp(unsigned long timestamp);
    unsigned long getMLModelTimestamp();
    
    // Factory Reset
    void factoryReset();
    
private:
    Preferences preferences;
    static constexpr const char* NAMESPACE = "waterpump";
    
    // Helper functions
    String generateCycleKey(int index);
    String generateDailyKey(unsigned long date);
    int getCurrentCycleIndex();
    void incrementCycleIndex();
};

#endif // STORAGE_MANAGER_H