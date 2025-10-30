// ============================================
// wifi_manager.h - FIXED VERSION
// ============================================
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <ESPmDNS.h>
#include "storage_manager.h"

// ✅ REMOVED custom WiFiMode enum - using ESP32's wifi_mode_t instead
// ESP32 provides: WIFI_MODE_NULL, WIFI_MODE_STA, WIFI_MODE_AP, WIFI_MODE_APSTA

struct WiFiNetwork {
    String ssid;
    int rssi;
    bool secured;
};

class WiFiManager {
public:
    WiFiManager();
    
    bool begin();
    void loop();
    
    // Station mode (connect to existing WiFi)
    bool connectToWiFi(const String& ssid, const String& password);
    bool connectToSavedWiFi();
    void disconnect();
    bool isConnected();
    
    // Access Point mode
    bool startAP(const String& ssid = "", const String& password = "");
    void stopAP();
    bool isAPActive();
    
    // Network scanning
    int scanNetworks(WiFiNetwork* networks, int maxNetworks);
    
    // Get network info
    String getSSID();
    String getIP();
    String getAPIP();
    int getRSSI();
    
    // mDNS
    bool startMDNS(const String& hostname);
    
    // Auto-reconnect
    void enableAutoReconnect(bool enable);
    wifi_mode_t getCurrentMode() const;
private:
    StorageManager _storage;
    wifi_mode_t _currentMode;  // ✅ FIXED: Use ESP32's wifi_mode_t
    bool _autoReconnect;
    unsigned long _lastReconnectAttempt;
    unsigned long _connectionStartTime;
    bool _apActive;
    String _currentSSID;
    String _currentPassword;
    
    void attemptReconnect();
};

#endif // WIFI_MANAGER_H