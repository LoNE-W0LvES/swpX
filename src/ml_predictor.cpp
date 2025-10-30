// ml_predictor.cpp
#include "ml_predictor.h"
#include "config.h"

MLPredictor::MLPredictor() 
    : _storage(nullptr),
      _modelLoaded(false),
      _enabled(ML_MODEL_ENABLED),
      _modelTimestamp(0),
      _lastUpdateCheck(0) {
}

bool MLPredictor::begin(StorageManager* storage, const String& serverUrl, const String& deviceToken) {
    _storage = storage;
    _serverUrl = serverUrl;
    _deviceToken = deviceToken;
    
    // Try to initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("SPIFFS initialization failed");
        #endif
        return false;
    }
    
    // Load model timestamp from storage
    if (_storage) {
        _modelTimestamp = _storage->getMLModelTimestamp();
    }
    
    // Try to load existing model
    if (SPIFFS.exists(ML_MODEL_PATH)) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("ML model found in SPIFFS");
        #endif
        
        if (loadModelFromSPIFFS()) {
            _modelLoaded = true;
            
            #if ENABLE_SERIAL_DEBUG
            Serial.println("ML Predictor initialized with existing model");
            #endif
            
            return true;
        }
    }
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("ML Predictor initialized (no model loaded - will use fallback)");
    #endif
    
    return true; // Return true even without model (graceful degradation)
}

bool MLPredictor::isReady() {
    return _modelLoaded && _enabled;
}

MLPrediction MLPredictor::predict(const MLInput& input) {
    // If model not loaded or disabled, use fallback prediction
    if (!_modelLoaded || !_enabled) {
        #if ENABLE_SERIAL_DEBUG && ML_FALLBACK_TO_AUTO
        Serial.println("ML model not available - using threshold-based fallback");
        #endif
        return fallbackPrediction(input);
    }
    
    // TODO: Implement TensorFlow Lite inference when library is added
    #if ENABLE_SERIAL_DEBUG
    Serial.println("ML prediction (TFLite not implemented yet - using fallback)");
    #endif
    
    return fallbackPrediction(input);
    
    /* 
    // Future implementation with TensorFlow Lite:
    
    // Prepare input tensor
    float normalizedInput[7];
    normalizeInput(normalizedInput, input);
    
    // Copy to input tensor
    TfLiteTensor* inputTensor = _interpreter->input(0);
    memcpy(inputTensor->data.f, normalizedInput, 7 * sizeof(float));
    
    // Run inference
    TfLiteStatus invokeStatus = _interpreter->Invoke();
    
    if (invokeStatus != kTfLiteOk) {
        return fallbackPrediction(input);
    }
    
    // Get output
    TfLiteTensor* outputTensor = _interpreter->output(0);
    
    MLPrediction prediction;
    prediction.shouldTurnOn = outputTensor->data.f[0] > 0.5;
    prediction.predictedNextFill = millis() + (unsigned long)(outputTensor->data.f[1] * 3600000);
    prediction.confidence = outputTensor->data.f[2];
    
    return prediction;
    */
}

bool MLPredictor::downloadModel() {
    // Check WiFi
    if (WiFi.status() != WL_CONNECTED) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Cannot download model - No WiFi");
        #endif
        return false;
    }
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Downloading ML model from server...");
    #endif
    
    String url = _serverUrl + "/api/ml/model";
    
    _http.begin(url);
    _http.addHeader("Authorization", "Bearer " + _deviceToken);
    
    int httpCode = _http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        #if ENABLE_SERIAL_DEBUG
        Serial.print("Model download failed. HTTP code: ");
        Serial.println(httpCode);
        #endif
        _http.end();
        return false;
    }
    
    int contentLength = _http.getSize();
    
    if (contentLength <= 0 || contentLength > 2000000) { // Max 2MB
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Invalid model size");
        #endif
        _http.end();
        return false;
    }
    
    WiFiClient* stream = _http.getStreamPtr();
    
    // Download to buffer
    uint8_t* modelData = (uint8_t*)malloc(contentLength);
    
    if (!modelData) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Failed to allocate memory for model");
        #endif
        _http.end();
        return false;
    }
    
    size_t written = 0;
    
    while (_http.connected() && (written < contentLength)) {
        size_t available = stream->available();
        
        if (available) {
            int c = stream->readBytes(modelData + written, available);
            written += c;
            
            #if ENABLE_SERIAL_DEBUG
            if (written % 10000 == 0) {
                Serial.print("Downloaded: ");
                Serial.print(written);
                Serial.print(" / ");
                Serial.println(contentLength);
            }
            #endif
        }
        delay(1);
    }
    
    _http.end();
    
    if (written != contentLength) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Model download incomplete");
        #endif
        free(modelData);
        return false;
    }
    
    // Save to SPIFFS
    bool saved = saveModelToSPIFFS(modelData, contentLength);
    free(modelData);
    
    if (saved) {
        // Load the new model
        if (loadModelFromSPIFFS()) {
            _modelLoaded = true;
            
            // Update timestamp
            _modelTimestamp = millis();
            if (_storage) {
                _storage->saveMLModelTimestamp(_modelTimestamp);
            }
            
            #if ENABLE_SERIAL_DEBUG
            Serial.println("ML model downloaded and loaded successfully");
            #endif
            
            return true;
        }
    }
    
    return false;
}

bool MLPredictor::needsModelUpdate() {
    // Check if it's been X days since last update
    unsigned long daysSinceUpdate = (millis() - _modelTimestamp) / 86400000;
    return daysSinceUpdate >= ML_MODEL_UPDATE_INTERVAL_DAYS;
}

void MLPredictor::setEnabled(bool enabled) {
    _enabled = enabled;
    
    #if ENABLE_SERIAL_DEBUG
    Serial.print("ML predictions ");
    Serial.println(enabled ? "enabled" : "disabled");
    #endif
}

bool MLPredictor::isEnabled() {
    return _enabled;
}

String MLPredictor::getModelVersion() {
    return _modelVersion;
}

unsigned long MLPredictor::getModelTimestamp() {
    return _modelTimestamp;
}

void MLPredictor::loop() {
    // Check for model updates periodically (only if WiFi available)
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }
    
    // Check once per day
    if (millis() - _lastUpdateCheck > 86400000) {
        _lastUpdateCheck = millis();
        
        if (needsModelUpdate()) {
            #if ENABLE_SERIAL_DEBUG
            Serial.println("ML model update needed, downloading...");
            #endif
            downloadModel();
        }
    }
}

bool MLPredictor::loadModelFromSPIFFS() {
    File file = SPIFFS.open(ML_MODEL_PATH, "r");
    
    if (!file) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Failed to open model file");
        #endif
        return false;
    }
    
    size_t fileSize = file.size();
    
    #if ENABLE_SERIAL_DEBUG
    Serial.print("Loading ML model (");
    Serial.print(fileSize);
    Serial.println(" bytes)...");
    #endif
    
    // TODO: Load TensorFlow Lite model when library is added
    
    file.close();
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("ML model loaded (TFLite not implemented - using fallback)");
    #endif
    
    return true;
}

bool MLPredictor::saveModelToSPIFFS(const uint8_t* data, size_t size) {
    File file = SPIFFS.open(ML_MODEL_PATH, "w");
    
    if (!file) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Failed to create model file");
        #endif
        return false;
    }
    
    size_t written = file.write(data, size);
    file.close();
    
    if (written != size) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Failed to write complete model to SPIFFS");
        #endif
        return false;
    }
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Model saved to SPIFFS successfully");
    #endif
    
    return true;
}

bool MLPredictor::fetchModelInfo() {
    String url = _serverUrl + "/api/ml/model/info";
    
    _http.begin(url);
    _http.addHeader("Authorization", "Bearer " + _deviceToken);
    
    int httpCode = _http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        _http.end();
        return false;
    }
    
    String payload = _http.getString();
    _http.end();
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        return false;
    }
    
    _modelVersion = doc["version"].as<String>();
    
    return true;
}

MLPrediction MLPredictor::fallbackPrediction(const MLInput& input) {
    // Simple threshold-based prediction (same as AUTO mode)
    MLPrediction prediction;
    
    // If current level is low, recommend turning on
    prediction.shouldTurnOn = (input.currentLevel < 20.0);
    
    // Estimate next fill time based on usage rate
    if (input.recentUsageRate > 0) {
        float hoursUntilEmpty = input.currentLevel / input.recentUsageRate;
        prediction.predictedNextFill = millis() + (unsigned long)(hoursUntilEmpty * 3600000);
    } else {
        prediction.predictedNextFill = millis() + 86400000; // 24 hours default
    }
    
    prediction.confidence = 0.5; // Low confidence for fallback
    
    return prediction;
}

void MLPredictor::normalizeInput(float* output, const MLInput& data) {
    // Normalize inputs to 0-1 range for neural network
    output[0] = data.hourOfDay / 23.0;
    output[1] = data.dayOfWeek / 6.0;
    output[2] = data.currentLevel / 100.0;
    output[3] = data.recentUsageRate / 100.0; // Assume max 100 L/hr
    output[4] = data.timeSinceLastFill / 1440.0; // Normalize to 24 hours
    output[5] = data.avgUsageSameHour / 50.0; // Assume max 50L avg
    output[6] = data.isWeekend ? 1.0 : 0.0;
}