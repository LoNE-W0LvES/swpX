// sync_manager.h
#ifndef SYNC_MANAGER_H
#define SYNC_MANAGER_H

#include "storage_manager.h"
#include "iot_client.h"
#include <ArduinoJson.h>

enum SyncStatus {
    SYNC_IDLE,
    SYNC_PENDING,
    SYNC_IN_PROGRESS,
    SYNC_SUCCESS,
    SYNC_FAILED,
    SYNC_CONFLICT
};

class SyncManager {
public:
    SyncManager();
    
    void begin(StorageManager* storage, IoTClient* iotClient);
    void loop();
    
    // Trigger sync operations
    bool syncConfig();
    bool pushConfig();
    bool pullConfig();
    
    // Handle config updates from device
    void onLocalConfigChange(const TankConfig& config);
    
    // Handle config updates from cloud
    void onCloudConfigReceived(const String& configJson);
    
    // Get sync status
    SyncStatus getStatus();
    String getStatusString();
    
    // Get last sync time
    unsigned long getLastSyncTime();
    
    // Force sync now
    void forceSyncNow();
    
    // Resolve conflicts manually
    bool resolveConflict(bool useCloudConfig);
    
private:
    StorageManager* _storage;
    IoTClient* _iotClient;
    
    SyncStatus _status;
    unsigned long _lastSyncTime;
    unsigned long _lastSyncAttempt;
    
    TankConfig _localConfig;
    TankConfig _cloudConfig;
    bool _conflictDetected;
    
    // Helper functions
    bool compareConfigs(const TankConfig& config1, const TankConfig& config2);
    TankConfig parseConfigJson(const String& json);
    void applyConfig(const TankConfig& config);
    bool validateConfig(const TankConfig& config);
};

#endif // SYNC_MANAGER_H
