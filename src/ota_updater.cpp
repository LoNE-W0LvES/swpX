// ota_updater.cpp
#include "ota_updater.h"
#include "config.h"

OTAUpdater::OTAUpdater() 
    : _storage(nullptr),
      _status(UPDATE_IDLE),
      _progress(0),
      _autoUpdateEnabled(AUTO_OTA_ENABLED),
      _lastCheckTime(0),
      _updateAvailable(false) {
}

bool OTAUpdater::begin(StorageManager* storage, const String& serverUrl, const String& deviceToken) {
    // Check if WiFi is available
    if (WiFi.status() != WL_CONNECTED) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("OTA Updater NOT initialized - No WiFi connection");
        #endif
        return false;
    }
    
    _storage = storage;
    _serverUrl = serverUrl;
    _deviceToken = deviceToken;
    
    // Load auto-update preference
    if (_storage) {
        _autoUpdateEnabled = _storage->isOTAEnabled();
    }
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("OTA Updater initialized");
    Serial.print("Auto-update: ");
    Serial.println(_autoUpdateEnabled ? "Enabled" : "Disabled");
    #endif
    
    return true;
}

bool OTAUpdater::checkForUpdate() {
    // Check if WiFi is available
    if (WiFi.status() != WL_CONNECTED) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Cannot check for updates - No WiFi");
        #endif
        _status = UPDATE_FAILED;
        return false;
    }
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Checking for firmware updates...");
    #endif
    
    _status = UPDATE_CHECKING;
    
    if (!fetchFirmwareInfo()) {
        _status = UPDATE_FAILED;
        return false;
    }
    
    // Compare versions
    String currentVersion = getCurrentVersion();
    
    if (isNewerVersion(_latestFirmware.version, currentVersion)) {
        _status = UPDATE_AVAILABLE;
        _updateAvailable = true;
        
        #if ENABLE_SERIAL_DEBUG
        Serial.print("Update available! Current: ");
        Serial.print(currentVersion);
        Serial.print(", Latest: ");
        Serial.println(_latestFirmware.version);
        #endif
        
        return true;
    } else {
        _status = UPDATE_NO_UPDATE;
        _updateAvailable = false;
        
        #if ENABLE_SERIAL_DEBUG
        Serial.println("No update available - Already on latest version");
        #endif
        
        return false;
    }
}

bool OTAUpdater::performUpdate() {
    if (!_updateAvailable) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("No update available to install");
        #endif
        return false;
    }
    
    // Check WiFi
    if (WiFi.status() != WL_CONNECTED) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Cannot update - No WiFi");
        #endif
        _status = UPDATE_FAILED;
        return false;
    }
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Starting firmware update...");
    Serial.print("Downloading from: ");
    Serial.println(_latestFirmware.downloadUrl);
    #endif
    
    _status = UPDATE_DOWNLOADING;
    
    if (downloadAndInstall(_latestFirmware.downloadUrl)) {
        _status = UPDATE_SUCCESS;
        
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Update successful! Rebooting in 3 seconds...");
        #endif
        
        delay(3000);
        ESP.restart();
        return true;
    } else {
        _status = UPDATE_FAILED;
        
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Update failed!");
        #endif
        
        return false;
    }
}

UpdateStatus OTAUpdater::getStatus() {
    return _status;
}

String OTAUpdater::getStatusString() {
    switch (_status) {
        case UPDATE_IDLE: return "Idle";
        case UPDATE_CHECKING: return "Checking...";
        case UPDATE_AVAILABLE: return "Update Available";
        case UPDATE_DOWNLOADING: return "Downloading...";
        case UPDATE_INSTALLING: return "Installing...";
        case UPDATE_SUCCESS: return "Success";
        case UPDATE_FAILED: return "Failed";
        case UPDATE_NO_UPDATE: return "Up to Date";
        default: return "Unknown";
    }
}

int OTAUpdater::getProgress() {
    return _progress;
}

FirmwareInfo OTAUpdater::getLatestFirmwareInfo() {
    return _latestFirmware;
}

void OTAUpdater::setAutoUpdate(bool enabled) {
    _autoUpdateEnabled = enabled;
    
    if (_storage) {
        _storage->saveOTAEnabled(enabled);
    }
    
    #if ENABLE_SERIAL_DEBUG
    Serial.print("Auto-update ");
    Serial.println(enabled ? "enabled" : "disabled");
    #endif
}

bool OTAUpdater::isAutoUpdateEnabled() {
    return _autoUpdateEnabled;
}

void OTAUpdater::loop() {
    // Only run if auto-update is enabled and WiFi available
    if (!_autoUpdateEnabled || WiFi.status() != WL_CONNECTED) {
        return;
    }
    
    // Check for updates periodically (once per day)
    if (millis() - _lastCheckTime > 86400000) { // 24 hours
        _lastCheckTime = millis();
        
        if (checkForUpdate()) {
            // Update available, perform update
            #if ENABLE_SERIAL_DEBUG
            Serial.println("Auto-update: Installing update...");
            #endif
            performUpdate();
        }
    }
}

bool OTAUpdater::fetchFirmwareInfo() {
    String url = _serverUrl + "/api/firmware/latest";
    
    _http.begin(url);
    _http.addHeader("Authorization", "Bearer " + _deviceToken);
    
    int httpCode = _http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        #if ENABLE_SERIAL_DEBUG
        Serial.print("Failed to fetch firmware info. HTTP code: ");
        Serial.println(httpCode);
        #endif
        _http.end();
        return false;
    }
    
    String payload = _http.getString();
    _http.end();
    
    // Parse JSON response
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        #if ENABLE_SERIAL_DEBUG
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
        #endif
        return false;
    }
    
    _latestFirmware.version = doc["version"].as<String>();
    _latestFirmware.downloadUrl = doc["downloadUrl"].as<String>();
    _latestFirmware.fileSize = doc["fileSize"] | 0;
    _latestFirmware.releaseNotes = doc["releaseNotes"].as<String>();
    _latestFirmware.md5Hash = doc["md5Hash"].as<String>();
    
    return true;
}

bool OTAUpdater::downloadAndInstall(const String& url) {
    _http.begin(url);
    _http.addHeader("Authorization", "Bearer " + _deviceToken);
    
    int httpCode = _http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        #if ENABLE_SERIAL_DEBUG
        Serial.print("Download failed. HTTP code: ");
        Serial.println(httpCode);
        #endif
        _http.end();
        return false;
    }
    
    int contentLength = _http.getSize();
    
    if (contentLength <= 0) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Invalid content length");
        #endif
        _http.end();
        return false;
    }
    
    bool canBegin = Update.begin(contentLength);
    
    if (!canBegin) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Not enough space for update");
        #endif
        _http.end();
        return false;
    }
    
    #if ENABLE_SERIAL_DEBUG
    Serial.print("Downloading firmware (");
    Serial.print(contentLength);
    Serial.println(" bytes)...");
    #endif
    
    _status = UPDATE_DOWNLOADING;
    
    WiFiClient* stream = _http.getStreamPtr();
    size_t written = 0;
    uint8_t buff[128] = { 0 };
    
    while (_http.connected() && (written < contentLength)) {
        size_t available = stream->available();
        
        if (available) {
            int c = stream->readBytes(buff, ((available > sizeof(buff)) ? sizeof(buff) : available));
            
            if (c > 0) {
                Update.write(buff, c);
                written += c;
                
                // Update progress
                _progress = (written * 100) / contentLength;
                
                #if ENABLE_SERIAL_DEBUG
                if (_progress % 10 == 0) {
                    Serial.print("Progress: ");
                    Serial.print(_progress);
                    Serial.println("%");
                }
                #endif
            }
        }
        delay(1);
    }
    
    _http.end();
    
    if (written != contentLength) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Download incomplete");
        #endif
        Update.abort();
        return false;
    }
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Download complete, installing...");
    #endif
    
    _status = UPDATE_INSTALLING;
    
    if (Update.end()) {
        if (Update.isFinished()) {
            #if ENABLE_SERIAL_DEBUG
            Serial.println("Update successfully installed");
            #endif
            return true;
        } else {
            #if ENABLE_SERIAL_DEBUG
            Serial.println("Update not finished");
            #endif
            return false;
        }
    } else {
        #if ENABLE_SERIAL_DEBUG
        Serial.print("Update error: ");
        Serial.println(Update.errorString());
        #endif
        return false;
    }
}

bool OTAUpdater::verifyUpdate() {
    // TODO: Verify MD5 hash if provided
    return true;
}

String OTAUpdater::getCurrentVersion() {
    return String(FIRMWARE_VERSION);
}

bool OTAUpdater::isNewerVersion(const String& newVersion, const String& currentVersion) {
    // Simple version comparison (assumes semantic versioning: X.Y.Z)
    int newMajor = 0, newMinor = 0, newPatch = 0;
    int curMajor = 0, curMinor = 0, curPatch = 0;
    
    sscanf(newVersion.c_str(), "%d.%d.%d", &newMajor, &newMinor, &newPatch);
    sscanf(currentVersion.c_str(), "%d.%d.%d", &curMajor, &curMinor, &curPatch);
    
    if (newMajor > curMajor) return true;
    if (newMajor < curMajor) return false;
    
    if (newMinor > curMinor) return true;
    if (newMinor < curMinor) return false;
    
    if (newPatch > curPatch) return true;
    
    return false;
}