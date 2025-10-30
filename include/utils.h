// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <time.h>

// ==================== TIME UTILITIES ====================

class TimeUtils {
public:
    // Format timestamp to readable string
    static String formatTimestamp(unsigned long timestamp);
    
    // Format duration in milliseconds to readable string
    static String formatDuration(unsigned long durationMs);
    
    // Get current time info
    static void getCurrentTime(int& hour, int& minute, int& second);
    static void getCurrentDate(int& year, int& month, int& day);
    static int getDayOfWeek(); // 0=Sunday, 6=Saturday
    
    // Check if current time is midnight
    static bool isMidnight();
    
    // Get midnight timestamp for today
    static unsigned long getTodayMidnight();
    
    // Time synchronization (NTP)
    static bool syncTimeNTP(const char* ntpServer = "pool.ntp.org");
    static bool isTimeSynced();
};

// ==================== STRING UTILITIES ====================

class StringUtils {
public:
    // Format float with specific precision
    static String formatFloat(float value, int decimals = 2);
    
    // Format percentage
    static String formatPercent(float value, int decimals = 1);
    
    // Format bytes to human readable (B, KB, MB)
    static String formatBytes(size_t bytes);
    
    // Trim whitespace
    static String trim(const String& str);
    
    // Convert to uppercase/lowercase
    static String toUpper(const String& str);
    static String toLower(const String& str);
    
    // String validation
    static bool isNumeric(const String& str);
    static bool isValidIP(const String& str);
    static bool isValidSSID(const String& str);
};

// ==================== MATH UTILITIES ====================

class MathUtils {
public:
    // clamp value between minVal and maxVal
    static float clamp(float value, float minVal, float maxVal);
    
    // Map value from one range to another
    static float map(float value, float inMin, float inMax, float outMin, float outMax);
    
    // Moving average filter
    static float movingAverage(float newValue, float* buffer, int bufferSize, int& index);
    
    // Exponential moving average
    static float exponentialMovingAverage(float newValue, float oldAverage, float alpha);
    
    // Calculate percentage
    static float percentage(float value, float total);
};

// ==================== VALIDATION UTILITIES ====================

class ValidationUtils {
public:
    // Validate tank configuration
    static bool validateTankHeight(float height);
    static bool validateTankDimensions(float length, float width, float radius);
    static bool validateThreshold(float upper, float lower);
    
    // Validate sensor readings
    static bool validateDistance(float distance);
    static bool validateWaterLevel(float level);
    
    // Validate network settings
    static bool validateSSID(const String& ssid);
    static bool validatePassword(const String& password);
};

// ==================== ERROR HANDLING ====================

enum ErrorCode {
    ERR_NONE = 0,
    ERR_SENSOR_FAIL = 1,
    ERR_STORAGE_FAIL = 2,
    ERR_WIFI_FAIL = 3,
    ERR_IOT_FAIL = 4,
    ERR_PUMP_FAIL = 5,
    ERR_CONFIG_INVALID = 6,
    ERR_OTA_FAIL = 7,
    ERR_ML_FAIL = 8
};

class ErrorHandler {
public:
    // Log error
    static void logError(ErrorCode code, const String& message);
    
    // Get error string
    static String getErrorString(ErrorCode code);
    
    // Clear error
    static void clearError(ErrorCode code);
    
    // Check if error exists
    static bool hasError(ErrorCode code);
};

// ==================== MEMORY UTILITIES ====================

class MemoryUtils {
public:
    // Get free heap memory
    static size_t getFreeHeap();
    
    // Get minimum free heap ever recorded
    static size_t getMinFreeHeap();
    
    // Get free PSRAM (if available)
    static size_t getFreePSRAM();
    
    // Print memory info
    static void printMemoryInfo();
    
    // Check if memory is critically low
    static bool isMemoryLow();
};

#endif // UTILS_H