// ota_updater.h
#ifndef OTA_UPDATER_H
#define OTA_UPDATER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <Update.h>
#include "storage_manager.h"

enum UpdateStatus {
    UPDATE_IDLE,
    UPDATE_CHECKING,
    UPDATE_AVAILABLE,
    UPDATE_DOWNLOADING,
    UPDATE_INSTALLING,
    UPDATE_SUCCESS,
    UPDATE_FAILED,
    UPDATE_NO_UPDATE
};

struct FirmwareInfo {
    String version;
    String downloadUrl;
    int fileSize;
    String releaseNotes;
    String md5Hash;
};

class OTAUpdater {
public:
    OTAUpdater();
    
    // Initialize (returns false if WiFi not available)
    bool begin(StorageManager* storage, const String& serverUrl, const String& deviceToken);
    
    // Check for updates (returns true if update available)
    bool checkForUpdate();
    
    // Download and install update
    bool performUpdate();
    
    // Get current status
    UpdateStatus getStatus();
    String getStatusString();
    
    // Get update progress (0-100)
    int getProgress();
    
    // Get latest firmware info
    FirmwareInfo getLatestFirmwareInfo();
    
    // Enable/disable automatic updates
    void setAutoUpdate(bool enabled);
    bool isAutoUpdateEnabled();
    
    // Should be called in loop() for automatic updates
    void loop();
    
private:
    StorageManager* _storage;
    String _serverUrl;
    String _deviceToken;
    HTTPClient _http;
    
    UpdateStatus _status;
    int _progress;
    FirmwareInfo _latestFirmware;
    bool _autoUpdateEnabled;
    unsigned long _lastCheckTime;
    bool _updateAvailable;
    
    // Helper functions
    bool fetchFirmwareInfo();
    bool downloadAndInstall(const String& url);
    bool verifyUpdate();
    String getCurrentVersion();
    bool isNewerVersion(const String& newVersion, const String& currentVersion);
};

#endif // OTA_UPDATER_H

