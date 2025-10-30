// ml_predictor.h
#ifndef ML_PREDICTOR_H
#define ML_PREDICTOR_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
// #include <TensorFlowLite_ESP32.h>  // Uncomment when TFLite library is added
#include "storage_manager.h"

struct MLInput {
    float hourOfDay;           // 0-23
    float dayOfWeek;           // 0-6 (0=Sunday)
    float currentLevel;        // 0-100
    float recentUsageRate;     // L/hour
    float timeSinceLastFill;   // minutes
    float avgUsageSameHour;    // historical average
    bool isWeekend;            // 0/1
};

struct MLPrediction {
    bool shouldTurnOn;         // Recommended pump action
    unsigned long predictedNextFill; // Timestamp
    float confidence;          // 0-1
};

class MLPredictor {
public:
    MLPredictor();
    
    // Initialize (returns false if model not available)
    bool begin(StorageManager* storage, const String& serverUrl, const String& deviceToken);
    
    // Check if ML model is loaded and ready
    bool isReady();
    
    // Make prediction based on current state
    MLPrediction predict(const MLInput& input);
    
    // Download model from server
    bool downloadModel();
    
    // Check if model update is needed
    bool needsModelUpdate();
    
    // Enable/disable ML predictions
    void setEnabled(bool enabled);
    bool isEnabled();
    
    // Get model info
    String getModelVersion();
    unsigned long getModelTimestamp();
    
    // Should be called in loop() for periodic model updates
    void loop();
    
private:
    StorageManager* _storage;
    String _serverUrl;
    String _deviceToken;
    HTTPClient _http;
    
    bool _modelLoaded;
    bool _enabled;
    unsigned long _modelTimestamp;
    String _modelVersion;
    unsigned long _lastUpdateCheck;
    
    // TensorFlow Lite objects (uncomment when library added)
    // tflite::MicroInterpreter* _interpreter;
    // const tflite::Model* _model;
    // uint8_t* _tensorArena;
    
    // Helper functions
    bool loadModelFromSPIFFS();
    bool saveModelToSPIFFS(const uint8_t* data, size_t size);
    bool fetchModelInfo();
    MLPrediction fallbackPrediction(const MLInput& input);
    void normalizeInput(float* input, const MLInput& data);
};

#endif // ML_PREDICTOR_H

