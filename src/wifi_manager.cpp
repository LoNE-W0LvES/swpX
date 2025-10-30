// ============================================
// wifi_manager.cpp - FIXED VERSION
// ============================================
#include "wifi_manager.h"
#include "config.h"

WiFiManager::WiFiManager() 
    : _currentMode(WIFI_MODE_NULL),  // ✅ FIXED: Use WIFI_MODE_NULL
      _autoReconnect(true),
      _lastReconnectAttempt(0),
      _connectionStartTime(0),
      _apActive(false) {
}

bool WiFiManager::begin() {
    if (!_storage.begin()) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Failed to initialize storage for WiFi manager");
        #endif
        return false;
    }
    
    WiFi.mode(WIFI_MODE_STA);
    _currentMode = WIFI_MODE_STA;
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("WiFi Manager initialized");
    #endif
    
    return true;
}

void WiFiManager::loop() {
    // Auto-reconnect logic
    if (_autoReconnect && _currentMode == WIFI_MODE_STA && !isConnected()) {
        if (millis() - _lastReconnectAttempt > WIFI_RECONNECT_INTERVAL_MS) {
            attemptReconnect();
        }
    }
}

bool WiFiManager::connectToWiFi(const String& ssid, const String& password) {
    if (ssid.isEmpty()) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Cannot connect: SSID is empty");
        #endif
        return false;
    }
    
    _currentSSID = ssid;
    _currentPassword = password;
    
    #if ENABLE_SERIAL_DEBUG
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    #endif
    
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    _connectionStartTime = millis();
    
    // Wait for connection with timeout
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - _connectionStartTime > WIFI_CONNECT_TIMEOUT_MS) {
            #if ENABLE_SERIAL_DEBUG
            Serial.println("WiFi connection timeout");
            #endif
            return false;
        }
        delay(100);
    }
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    #endif
    
    // Save credentials
    _storage.saveWiFiCredentials(ssid, password);
    
    _currentMode = WIFI_MODE_STA;
    return true;
}

bool WiFiManager::connectToSavedWiFi() {
    String ssid, password;
    
    if (!_storage.loadWiFiCredentials(ssid, password)) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("No saved WiFi credentials found");
        #endif
        return false;
    }
    
    return connectToWiFi(ssid, password);
}

void WiFiManager::disconnect() {
    WiFi.disconnect();
    _currentMode = WIFI_MODE_NULL;  // ✅ FIXED: Use WIFI_MODE_NULL
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("WiFi disconnected");
    #endif
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiManager::startAP(const String& ssid, const String& password) {
    String apSSID = ssid.isEmpty() ? AP_SSID : ssid;
    String apPassword = password.isEmpty() ? AP_PASSWORD : password;
    
    #if ENABLE_SERIAL_DEBUG
    Serial.print("Starting Access Point: ");
    Serial.println(apSSID);
    #endif
    
    if (_currentMode == WIFI_MODE_STA) {
        WiFi.mode(WIFI_MODE_APSTA);
        _currentMode = WIFI_MODE_APSTA;  // ✅ FIXED: Use WIFI_MODE_APSTA
    } else {
        WiFi.mode(WIFI_MODE_AP);
        _currentMode = WIFI_MODE_AP;
    }
    
    bool success = WiFi.softAP(apSSID.c_str(), apPassword.c_str());
    
    if (success) {
        _apActive = true;
        
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Access Point started");
        Serial.print("AP IP address: ");
        Serial.println(WiFi.softAPIP());
        #endif
    } else {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Failed to start Access Point");
        #endif
    }
    
    return success;
}

void WiFiManager::stopAP() {
    WiFi.softAPdisconnect(true);
    _apActive = false;
    
    if (_currentMode == WIFI_MODE_APSTA) {  // ✅ FIXED: Use WIFI_MODE_APSTA
        WiFi.mode(WIFI_MODE_STA);
        _currentMode = WIFI_MODE_STA;
    } else {
        WiFi.mode(WIFI_MODE_NULL);
        _currentMode = WIFI_MODE_NULL;  // ✅ FIXED: Use WIFI_MODE_NULL
    }
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Access Point stopped");
    #endif
}

bool WiFiManager::isAPActive() {
    return _apActive;
}

int WiFiManager::scanNetworks(WiFiNetwork* networks, int maxNetworks) {
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Scanning WiFi networks...");
    #endif
    
    int n = WiFi.scanNetworks();
    
    if (n == 0) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("No networks found");
        #endif
        return 0;
    }
    
    int count = min(n, maxNetworks);
    
    for (int i = 0; i < count; i++) {
        networks[i].ssid = WiFi.SSID(i);
        networks[i].rssi = WiFi.RSSI(i);
        networks[i].secured = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
        
        #if ENABLE_SERIAL_DEBUG
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(networks[i].ssid);
        Serial.print(" (");
        Serial.print(networks[i].rssi);
        Serial.print(" dBm) ");
        Serial.println(networks[i].secured ? "[SECURED]" : "[OPEN]");
        #endif
    }
    
    WiFi.scanDelete();
    return count;
}

String WiFiManager::getSSID() {
    return WiFi.SSID();
}

String WiFiManager::getIP() {
    return WiFi.localIP().toString();
}

String WiFiManager::getAPIP() {
    return WiFi.softAPIP().toString();
}

int WiFiManager::getRSSI() {
    return WiFi.RSSI();
}

bool WiFiManager::startMDNS(const String& hostname) {
    if (!MDNS.begin(hostname.c_str())) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Error setting up mDNS responder");
        #endif
        return false;
    }
    
    #if ENABLE_SERIAL_DEBUG
    Serial.print("mDNS responder started: ");
    Serial.print(hostname);
    Serial.println(".local");
    #endif
    
    return true;
}

void WiFiManager::enableAutoReconnect(bool enable) {
    _autoReconnect = enable;
    
    #if ENABLE_SERIAL_DEBUG
    Serial.print("Auto-reconnect ");
    Serial.println(enable ? "enabled" : "disabled");
    #endif
}

void WiFiManager::attemptReconnect() {
    _lastReconnectAttempt = millis();
    
    if (_currentSSID.isEmpty()) {
        // Try to connect to saved WiFi
        String ssid, password;
        if (_storage.loadWiFiCredentials(ssid, password)) {
            _currentSSID = ssid;
            _currentPassword = password;
        } else {
            return; // No credentials to reconnect with
        }
    }
    
    #if ENABLE_SERIAL_DEBUG
    Serial.print("Attempting to reconnect to: ");
    Serial.println(_currentSSID);
    #endif
    
    WiFi.begin(_currentSSID.c_str(), _currentPassword.c_str());
}

wifi_mode_t WiFiManager::getCurrentMode() const {
    return _currentMode;
}
// ============================================
// KEY CHANGES SUMMARY:
// ============================================
// 1. REMOVED custom WiFiMode enum from .h file
// 2. CHANGED _currentMode type from WiFiMode to wifi_mode_t
// 3. REPLACED all custom enum values with ESP32 constants:
//    - WIFI_MODE_OFF    → WIFI_MODE_NULL
//    - WIFI_MODE_STA    → WIFI_MODE_STA (same)
//    - WIFI_MODE_AP     → WIFI_MODE_AP (same)
//    - WIFI_MODE_AP_STA → WIFI_MODE_APSTA
// ===========================================