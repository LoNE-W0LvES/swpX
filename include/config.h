// config.h - All configurable constants for Smart Water Pump System
#ifndef CONFIG_H
#define CONFIG_H

// ==================== SIMULATION MODE ====================
// Enable this when running in Wokwi simulator (disables AP mode only)
// Note: Wokwi supports WiFi connection to "Wokwi-GUEST" but not AP mode
#define SIMULATION_MODE true               // Set to false for real hardware

// ==================== PROTOCOL DEFINITIONS (MUST BE FIRST) ====================
#define PROTOCOL_MQTT 1
#define PROTOCOL_WEBSOCKET 2
#define PROTOCOL_RESTAPI 3

// ==================== COMMUNICATION PROTOCOL ====================
// Options: PROTOCOL_MQTT, PROTOCOL_WEBSOCKET, PROTOCOL_RESTAPI
#define COMMUNICATION_PROTOCOL PROTOCOL_MQTT

// ==================== SENSOR CONFIGURATION ====================
#define SENSOR_DEAD_ZONE_CM 25              // Unusable distance from sensor (0-25cm)
#define SENSOR_SAMPLE_INTERVAL_MS 5000      // Sensor reading interval (5 seconds)
#define SENSOR_MAX_RETRIES 3                // Retry count for failed readings
#define SENSOR_TIMEOUT_MS 1000              // Ultrasonic timeout

// ==================== PUMP SAFETY FEATURES ====================
#define ENABLE_DRY_RUN_PROTECTION true      // Prevent pump running without water increase
#define DRY_RUN_TIMEOUT_MINUTES 5           // Minutes before dry-run detection
#define ENABLE_OVERFLOW_PROTECTION true     // Emergency shutoff near full
#define OVERFLOW_EMERGENCY_LEVEL 98.0       // Percentage for emergency stop
#define ENABLE_RAPID_CYCLE_PROTECTION true  // Prevent rapid on/off
#define MINIMUM_RUN_TIME_SECONDS 60         // Minimum pump run time
#define MINIMUM_OFF_TIME_SECONDS 120        // Minimum pump off time

// ==================== WATER LEVEL THRESHOLDS ====================
#define DEFAULT_UPPER_THRESHOLD 90.0        // Default upper threshold (%)
#define DEFAULT_LOWER_THRESHOLD 20.0        // Default lower threshold (%)
#define MANUAL_OVERRIDE_MAX_LEVEL 95.0      // Max level for manual override

// ==================== NETWORK CONFIGURATION ====================
#define AP_SSID "SmartWaterPump"            // Access Point name
#define AP_PASSWORD "pump12345"             // Access Point password (min 8 chars)
#define WEBSERVER_PORT 80                   // Local web server port
#define WIFI_CONNECT_TIMEOUT_MS 20000       // WiFi connection timeout
#define WIFI_RECONNECT_INTERVAL_MS 60000    // Retry interval after failure

// ==================== IOT SERVER CONFIGURATION ====================
// IMPORTANT: System works WITHOUT internet/server connection
// IoT features are optional and will gracefully disable if server unavailable

// Set to false for standalone mode (no cloud connectivity required)
#define IOT_ENABLED false  // ✅ CHANGED: Start with IoT disabled

// For production: Update these with your actual server details
#define IOT_SERVER_URL "http://192.168.1.100:3000"  // Local server for testing
#define IOT_MQTT_BROKER "192.168.1.100"             // Local MQTT broker
#define IOT_MQTT_PORT 1883                          // MQTT broker port
#define IOT_WEBSOCKET_URL "ws://192.168.1.100:3000" // WebSocket URL
#define IOT_API_ENDPOINT "/api/v1/devices"          // REST API endpoint
#define IOT_CONNECTION_TIMEOUT_MS 10000             // Timeout for IoT attempts
#define IOT_RETRY_AFTER_FAIL_MS 300000              // Retry after 5 minutes

// ==================== UPDATE & SYNC CONFIGURATION ====================
// OTA and sync are OPTIONAL - system works standalone
#define AUTO_OTA_ENABLED false              // Disable until server ready
#define OTA_CHECK_AT_STARTUP false          // Don't check at boot
#define OTA_CHECK_DAILY false               // Don't check daily
#define CONFIG_SYNC_INTERVAL_MS 300000      // Sync every 5 minutes (if connected)
#define TELEMETRY_SEND_INTERVAL_MS 10000    // Send telemetry every 10s (if connected)
#define GRACEFUL_IOT_FAILURE true           // Continue if IoT fails ✅

// ==================== SYNC MODE ====================
#define DEFAULT_SYNC_MODE DEVICE_PRIORITY   // ✅ FIXED: Use enum value directly

// ==================== ML MODEL CONFIGURATION ====================
// ML model is OPTIONAL - system works without it
#define ML_MODEL_ENABLED false              // Disabled until model available
#define ML_MODEL_UPDATE_INTERVAL_DAYS 7     // Download new model every N days
#define ML_MODEL_PATH "/model.tflite"       // Model file path in SPIFFS
#define ML_PREDICTION_INTERVAL_MS 60000     // Run prediction every minute
#define ML_FALLBACK_TO_AUTO true            // Use AUTO mode if ML not available ✅

// ==================== DISPLAY CONFIGURATION ====================
#define DISPLAY_UPDATE_INTERVAL_MS 1000     // Update display every second
#define DISPLAY_TIMEOUT_SECONDS 300         // Dim display after 5 min inactivity
#define SCREEN_ROTATION_DELAY_MS 5000       // Auto-rotate screens every 5 sec

// ==================== STORAGE CONFIGURATION ====================
#define PREFERENCES_NAMESPACE "waterpump"   // Preferences namespace
#define MAX_DAILY_RECORDS 30                // Store 30 days of history
#define MAX_PUMP_CYCLE_LOGS 100             // Store last 100 pump cycles

// ==================== WEB DASHBOARD CONFIGURATION ====================
#define WEB_UPDATE_INTERVAL_MS 2000         // Update web dashboard every 2s
#define WEB_ENABLE_AUTH false               // ✅ Disabled for easier testing
#define WEB_DEFAULT_USERNAME "admin"        // Default username
#define WEB_DEFAULT_PASSWORD "admin"        // Default password

// ==================== BUTTON CONFIGURATION ====================
#define BUTTON_DEBOUNCE_MS 50               // Button debounce time
#define BUTTON_LONG_PRESS_MS 5000           // Long press duration for override
#define CONFIG_MODE_TIMEOUT_MS 300000       // Exit config mode after 5 min

// ==================== DEBUG CONFIGURATION ====================
#define ENABLE_SERIAL_DEBUG true            // Enable serial debugging ✅
#define SERIAL_BAUD_RATE 115200             // Serial baud rate

// ==================== FIRMWARE VERSION ====================
#define FIRMWARE_VERSION "1.0.0"            // Current firmware version
#define HARDWARE_VERSION "ESP32-S3-N8R16"   // Hardware identifier

#endif // CONFIG_H