// sync_manager.cpp
#include "sync_manager.h"
#include "config.h"

SyncManager::SyncManager() 
    : _storage(nullptr),
      _iotClient(nullptr),
      _status(SYNC_IDLE),
      _lastSyncTime(0),
      _lastSyncAttempt(0),
      _conflictDetected(false) {
}

void SyncManager::begin(StorageManager* storage, IoTClient* iotClient) {
    _storage = storage;
    _iotClient = iotClient;
    
    // Load current local config
    _localConfig = _storage->loadTankConfig();
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Sync manager initialized");
    Serial.print("Sync mode: ");
    Serial.println(_localConfig.syncMode == DEVICE_PRIORITY ? "DEVICE_PRIORITY" : "CLOUD_PRIORITY");
    #endif
}

void SyncManager::loop() {
    // Periodic sync check
    if (millis() - _lastSyncAttempt > CONFIG_SYNC_INTERVAL_MS) {
        _lastSyncAttempt = millis();
        
        if (_iotClient && _iotClient->isConnected()) {
            if (_localConfig.needsSync) {
                syncConfig();
            }
        }
    }
}

bool SyncManager::syncConfig() {
    if (!_iotClient || !_iotClient->isConnected()) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Cannot sync: Not connected to IoT");
        #endif
        _status = SYNC_FAILED;
        return false;
    }
    
    _status = SYNC_IN_PROGRESS;
    
    // Reload latest local config
    _localConfig = _storage->loadTankConfig();
    
    if (_localConfig.syncMode == DEVICE_PRIORITY) {
        // Device priority: push local config to cloud
        if (_localConfig.needsSync) {
            #if ENABLE_SERIAL_DEBUG
            Serial.println("Syncing (DEVICE_PRIORITY): Pushing config to cloud");
            #endif
            
            if (pushConfig()) {
                _storage->markNeedsSync(false);
                _status = SYNC_SUCCESS;
                _lastSyncTime = millis();
                return true;
            } else {
                _status = SYNC_FAILED;
                return false;
            }
        }
    } else {
        // Cloud priority: pull config from cloud
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Syncing (CLOUD_PRIORITY): Pulling config from cloud");
        #endif
        
        if (pullConfig()) {
            _status = SYNC_SUCCESS;
            _lastSyncTime = millis();
            return true;
        } else {
            _status = SYNC_FAILED;
            return false;
        }
    }
    
    _status = SYNC_IDLE;
    return true;
}

bool SyncManager::pushConfig() {
    if (!_iotClient) return false;
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Pushing config to cloud...");
    #endif
    
    if (_iotClient->sendConfig(_localConfig)) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Config pushed successfully");
        #endif
        return true;
    } else {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Failed to push config");
        #endif
        return false;
    }
}

bool SyncManager::pullConfig() {
    if (!_iotClient) return false;
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Requesting config from cloud...");
    #endif
    
    // Request config from cloud
    // The actual config will arrive via callback
    return _iotClient->requestConfig();
}

void SyncManager::onLocalConfigChange(const TankConfig& config) {
    _localConfig = config;
    
    // Update version and mark for sync
    _storage->updateConfigVersion("device");
    _storage->markNeedsSync(true);
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Local config changed, marked for sync");
    #endif
    
    // Immediate sync if connected
    if (_iotClient && _iotClient->isConnected()) {
        syncConfig();
    }
}

void SyncManager::onCloudConfigReceived(const String& configJson) {
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Cloud config received");
    #endif
    
    _cloudConfig = parseConfigJson(configJson);
    
    // Validate cloud config
    if (!validateConfig(_cloudConfig)) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Invalid cloud config received");
        #endif
        return;
    }
    
    // Compare versions
    if (_cloudConfig.configVersion > _localConfig.configVersion) {
        if (_localConfig.syncMode == CLOUD_PRIORITY) {
            // Apply cloud config immediately
            #if ENABLE_SERIAL_DEBUG
            Serial.println("Applying cloud config (CLOUD_PRIORITY)");
            #endif
            applyConfig(_cloudConfig);
            _status = SYNC_SUCCESS;
            _lastSyncTime = millis();
        } else {
            // Device priority but cloud has newer version - conflict!
            if (_localConfig.needsSync) {
                _conflictDetected = true;
                _status = SYNC_CONFLICT;
                
                #if ENABLE_SERIAL_DEBUG
                Serial.println("CONFLICT: Both local and cloud configs have changes");
                #endif
            } else {
                // No local changes pending, safe to apply cloud config
                applyConfig(_cloudConfig);
                _status = SYNC_SUCCESS;
                _lastSyncTime = millis();
            }
        }
    } else if (_cloudConfig.configVersion < _localConfig.configVersion) {
        // Local config is newer, push to cloud
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Local config is newer, pushing to cloud");
        #endif
        pushConfig();
    } else {
        // Same version, configs are in sync
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Configs are in sync");
        #endif
        _status = SYNC_SUCCESS;
        _storage->markNeedsSync(false);
    }
}

SyncStatus SyncManager::getStatus() {
    return _status;
}

String SyncManager::getStatusString() {
    switch (_status) {
        case SYNC_IDLE: return "Idle";
        case SYNC_PENDING: return "Pending";
        case SYNC_IN_PROGRESS: return "Syncing...";
        case SYNC_SUCCESS: return "Success";
        case SYNC_FAILED: return "Failed";
        case SYNC_CONFLICT: return "Conflict";
        default: return "Unknown";
    }
}

unsigned long SyncManager::getLastSyncTime() {
    return _lastSyncTime;
}

void SyncManager::forceSyncNow() {
    _lastSyncAttempt = 0; // Reset timer to force immediate sync
    syncConfig();
}

bool SyncManager::resolveConflict(bool useCloudConfig) {
    if (!_conflictDetected) return false;
    
    if (useCloudConfig) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Resolving conflict: Using cloud config");
        #endif
        applyConfig(_cloudConfig);
    } else {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Resolving conflict: Keeping local config and pushing to cloud");
        #endif
        pushConfig();
    }
    
    _conflictDetected = false;
    _status = SYNC_SUCCESS;
    return true;
}

bool SyncManager::compareConfigs(const TankConfig& config1, const TankConfig& config2) {
    return (config1.tankHeight == config2.tankHeight &&
            config1.tankLength == config2.tankLength &&
            config1.tankWidth == config2.tankWidth &&
            config1.tankRadius == config2.tankRadius &&
            config1.shape == config2.shape &&
            config1.upperThreshold == config2.upperThreshold &&
            config1.lowerThreshold == config2.lowerThreshold);
}

TankConfig SyncManager::parseConfigJson(const String& json) {
    TankConfig config;
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        #if ENABLE_SERIAL_DEBUG
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
        #endif
        return config;
    }
    
    config.tankHeight = doc["tankHeight"] | 0.0f;
    config.tankLength = doc["tankLength"] | 0.0f;
    config.tankWidth = doc["tankWidth"] | 0.0f;
    config.tankRadius = doc["tankRadius"] | 0.0f;
    
    String shape = doc["shape"] | "rectangular";
    config.shape = (shape == "cylindrical") ? CYLINDRICAL : RECTANGULAR;
    
    config.upperThreshold = doc["upperThreshold"] | DEFAULT_UPPER_THRESHOLD;
    config.lowerThreshold = doc["lowerThreshold"] | DEFAULT_LOWER_THRESHOLD;
    config.maxInflow = doc["maxInflow"] | 0.0f;
    config.configVersion = doc["configVersion"] | 0;
    config.lastModifiedSource = doc["lastModifiedSource"] | "cloud";
    
    String syncMode = doc["syncMode"] | "device";
    config.syncMode = (syncMode == "cloud") ? CLOUD_PRIORITY : DEVICE_PRIORITY;
    
    return config;
}

void SyncManager::applyConfig(const TankConfig& config) {
    _localConfig = config;
    _storage->saveTankConfig(config);
    _storage->markNeedsSync(false);
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Config applied and saved");
    #endif
}

bool SyncManager::validateConfig(const TankConfig& config) {
    if (config.tankHeight <= 0) return false;
    
    if (config.shape == RECTANGULAR) {
        if (config.tankLength <= 0 || config.tankWidth <= 0) return false;
    } else if (config.shape == CYLINDRICAL) {
        if (config.tankRadius <= 0) return false;
    }
    
    if (config.upperThreshold <= config.lowerThreshold) return false;
    if (config.upperThreshold > 100 || config.lowerThreshold < 0) return false;
    
    return true;
}