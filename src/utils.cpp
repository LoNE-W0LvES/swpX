// utils.cpp
#include "utils.h"
#include "config.h"

// ==================== TIME UTILITIES IMPLEMENTATION ====================

String TimeUtils::formatTimestamp(unsigned long timestamp) {
    time_t rawtime = timestamp;
    struct tm* timeinfo = localtime(&rawtime);
    
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    return String(buffer);
}

String TimeUtils::formatDuration(unsigned long durationMs) {
    unsigned long seconds = durationMs / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
    
    String result = "";
    
    if (days > 0) result += String(days) + "d ";
    if (hours > 0) result += String(hours) + "h ";
    if (minutes > 0) result += String(minutes) + "m ";
    result += String(seconds) + "s";
    
    return result;
}

void TimeUtils::getCurrentTime(int& hour, int& minute, int& second) {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    hour = timeinfo->tm_hour;
    minute = timeinfo->tm_min;
    second = timeinfo->tm_sec;
}

void TimeUtils::getCurrentDate(int& year, int& month, int& day) {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    year = timeinfo->tm_year + 1900;
    month = timeinfo->tm_mon + 1;
    day = timeinfo->tm_mday;
}

int TimeUtils::getDayOfWeek() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    return timeinfo->tm_wday;
}

bool TimeUtils::isMidnight() {
    int hour, minute, second;
    getCurrentTime(hour, minute, second);
    return (hour == 0 && minute == 0);
}

unsigned long TimeUtils::getTodayMidnight() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    timeinfo->tm_hour = 0;
    timeinfo->tm_min = 0;
    timeinfo->tm_sec = 0;
    
    return mktime(timeinfo);
}

bool TimeUtils::syncTimeNTP(const char* ntpServer) {
    configTime(0, 0, ntpServer);
    
    // Wait up to 5 seconds for time sync
    int timeout = 50;
    while (time(nullptr) < 100000 && timeout-- > 0) {
        delay(100);
    }
    
    return isTimeSynced();
}

bool TimeUtils::isTimeSynced() {
    return time(nullptr) > 100000;
}

// ==================== STRING UTILITIES IMPLEMENTATION ====================

String StringUtils::formatFloat(float value, int decimals) {
    return String(value, decimals);
}

String StringUtils::formatPercent(float value, int decimals) {
    return String(value, decimals) + "%";
}

String StringUtils::formatBytes(size_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    else if (bytes < 1048576) return String(bytes / 1024.0, 2) + " KB";
    else return String(bytes / 1048576.0, 2) + " MB";
}

String StringUtils::trim(const String& str) {
    String result = str;
    result.trim();
    return result;
}

String StringUtils::toUpper(const String& str) {
    String result = str;
    result.toUpperCase();
    return result;
}

String StringUtils::toLower(const String& str) {
    String result = str;
    result.toLowerCase();
    return result;
}

bool StringUtils::isNumeric(const String& str) {
    for (unsigned int i = 0; i < str.length(); i++) {
        if (!isDigit(str.charAt(i)) && str.charAt(i) != '.' && str.charAt(i) != '-') {
            return false;
        }
    }
    return true;
}

bool StringUtils::isValidIP(const String& str) {
    int dots = 0;
    for (unsigned int i = 0; i < str.length(); i++) {
        if (str.charAt(i) == '.') dots++;
        else if (!isDigit(str.charAt(i))) return false;
    }
    return dots == 3;
}

bool StringUtils::isValidSSID(const String& str) {
    return str.length() > 0 && str.length() <= 32;
}

// ==================== MATH UTILITIES IMPLEMENTATION ====================

float MathUtils::clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

float MathUtils::map(float value, float inMin, float inMax, float outMin, float outMax) {
    return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

float MathUtils::movingAverage(float newValue, float* buffer, int bufferSize, int& index) {
    buffer[index] = newValue;
    index = (index + 1) % bufferSize;
    
    float sum = 0;
    for (int i = 0; i < bufferSize; i++) {
        sum += buffer[i];
    }
    
    return sum / bufferSize;
}

float MathUtils::exponentialMovingAverage(float newValue, float oldAverage, float alpha) {
    return alpha * newValue + (1.0 - alpha) * oldAverage;
}

float MathUtils::percentage(float value, float total) {
    if (total == 0) return 0;
    return (value / total) * 100.0;
}

// ==================== VALIDATION UTILITIES IMPLEMENTATION ====================

bool ValidationUtils::validateTankHeight(float height) {
    return height > 0 && height <= 1000; // Max 10 meters
}

bool ValidationUtils::validateTankDimensions(float length, float width, float radius) {
    if (radius > 0) {
        return radius > 0 && radius <= 500; // Max 5 meters radius
    } else {
        return length > 0 && length <= 1000 && width > 0 && width <= 1000;
    }
}

bool ValidationUtils::validateThreshold(float upper, float lower) {
    return upper > lower && lower >= 0 && upper <= 100;
}

bool ValidationUtils::validateDistance(float distance) {
    return distance >= 0 && distance <= 400; // JSN-SR04T max range
}

bool ValidationUtils::validateWaterLevel(float level) {
    return level >= 0 && level <= 100;
}

bool ValidationUtils::validateSSID(const String& ssid) {
    return ssid.length() > 0 && ssid.length() <= 32;
}

bool ValidationUtils::validatePassword(const String& password) {
    return password.length() == 0 || password.length() >= 8;
}

// ==================== ERROR HANDLING IMPLEMENTATION ====================

static uint8_t errorFlags = 0;

void ErrorHandler::logError(ErrorCode code, const String& message) {
    errorFlags |= (1 << code);
    
    #if ENABLE_SERIAL_DEBUG
    Serial.print("ERROR [");
    Serial.print(code);
    Serial.print("]: ");
    Serial.println(message);
    #endif
}

String ErrorHandler::getErrorString(ErrorCode code) {
    switch (code) {
        case ERR_NONE: return "No error";
        case ERR_SENSOR_FAIL: return "Sensor failure";
        case ERR_STORAGE_FAIL: return "Storage failure";
        case ERR_WIFI_FAIL: return "WiFi failure";
        case ERR_IOT_FAIL: return "IoT connection failure";
        case ERR_PUMP_FAIL: return "Pump failure";
        case ERR_CONFIG_INVALID: return "Invalid configuration";
        case ERR_OTA_FAIL: return "OTA update failure";
        case ERR_ML_FAIL: return "ML model failure";
        default: return "Unknown error";
    }
}

void ErrorHandler::clearError(ErrorCode code) {
    errorFlags &= ~(1 << code);
}

bool ErrorHandler::hasError(ErrorCode code) {
    return errorFlags & (1 << code);
}

// ==================== MEMORY UTILITIES IMPLEMENTATION ====================

size_t MemoryUtils::getFreeHeap() {
    return ESP.getFreeHeap();
}

size_t MemoryUtils::getMinFreeHeap() {
    return ESP.getMinFreeHeap();
}

size_t MemoryUtils::getFreePSRAM() {
    return ESP.getFreePsram();
}

void MemoryUtils::printMemoryInfo() {
    #if ENABLE_SERIAL_DEBUG
    Serial.println("=== Memory Info ===");
    Serial.print("Free Heap: ");
    Serial.println(StringUtils::formatBytes(getFreeHeap()));
    Serial.print("Min Free Heap: ");
    Serial.println(StringUtils::formatBytes(getMinFreeHeap()));
    Serial.print("Free PSRAM: ");
    Serial.println(StringUtils::formatBytes(getFreePSRAM()));
    Serial.println("==================");
    #endif
}

bool MemoryUtils::isMemoryLow() {
    return getFreeHeap() < 50000; // Less than 50KB is considered low
}