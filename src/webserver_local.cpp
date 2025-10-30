// webserver_local.cpp
#include "webserver_local.h"
#include "config.h"

WebServerLocal::WebServerLocal() 
    : _server(nullptr),
      _storage(nullptr),
      _calculator(nullptr),
      _pump(nullptr),
      _tracker(nullptr),
      _isRunning(false),
      _waterLevel(0),
      _currentInflow(0),
      _maxInflow(0) {
}

bool WebServerLocal::begin(StorageManager* storage, TankCalculator* calculator, 
                           PumpController* pump, WaterTracker* tracker) {
    // Check if WiFi is available
    if (WiFi.status() != WL_CONNECTED && WiFi.softAPgetStationNum() == 0) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Web server NOT started - No WiFi connection");
        #endif
        return false;
    }
    
    _storage = storage;
    _calculator = calculator;
    _pump = pump;
    _tracker = tracker;
    
    // Create server instance
    _server = new AsyncWebServer(WEBSERVER_PORT);
    
    if (!_server) {
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Failed to create web server");
        #endif
        return false;
    }
    
    // Setup routes
    setupRoutes();
    
    // Start server
    _server->begin();
    _isRunning = true;
    
    #if ENABLE_SERIAL_DEBUG
    Serial.println("Web server started successfully");
    Serial.print("Access at: http://");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print(WiFi.localIP());
    } else {
        Serial.print(WiFi.softAPIP());
    }
    Serial.print(":");
    Serial.println(WEBSERVER_PORT);
    #endif
    
    return true;
}

void WebServerLocal::updateData(float waterLevel, float currentInflow, float maxInflow) {
    _waterLevel = waterLevel;
    _currentInflow = currentInflow;
    _maxInflow = maxInflow;
}

bool WebServerLocal::isRunning() {
    return _isRunning;
}

void WebServerLocal::stop() {
    if (_server && _isRunning) {
        _server->end();
        delete _server;
        _server = nullptr;
        _isRunning = false;
        
        #if ENABLE_SERIAL_DEBUG
        Serial.println("Web server stopped");
        #endif
    }
}

void WebServerLocal::setupRoutes() {
    // Dashboard
    _server->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleRoot(request);
    });
    
    // API endpoints
    _server->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleStatus(request);
    });
    
    _server->on("/api/telemetry", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleTelemetry(request);
    });
    
    _server->on("/api/pump/on", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handlePumpOn(request);
    });
    
    _server->on("/api/pump/off", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handlePumpOff(request);
    });
    
    _server->on("/api/mode", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleSetMode(request);
    });
    
    _server->on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetConfig(request);
    });
    
    _server->on("/api/config", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleSetConfig(request);
    });
    
    _server->on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleWiFiScan(request);
    });
    
    _server->on("/api/wifi/connect", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleWiFiConnect(request);
    });
    
    _server->on("/api/setup", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleSetup(request);
    });
    
    _server->on("/api/usage", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleUsageStats(request);
    });
    
    // 404 handler
    _server->onNotFound([](AsyncWebServerRequest* request) {
        request->send(404, "application/json", "{\"error\":\"Not found\"}");
    });
}

void WebServerLocal::handleRoot(AsyncWebServerRequest* request) {
    const char html[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Smart Water Pump</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: Arial, sans-serif; background: #1a1a2e; color: #eee; padding: 20px; }
        .container { max-width: 800px; margin: 0 auto; }
        h1 { text-align: center; margin-bottom: 30px; color: #4fc3f7; }
        .card { background: #16213e; padding: 20px; border-radius: 10px; margin-bottom: 20px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); }
        .level-display { text-align: center; font-size: 48px; font-weight: bold; color: #4fc3f7; margin: 20px 0; }
        .tank { width: 200px; height: 300px; border: 3px solid #4fc3f7; border-radius: 10px; margin: 20px auto; position: relative; overflow: hidden; }
        .water { position: absolute; bottom: 0; width: 100%; background: linear-gradient(to top, #1e88e5, #4fc3f7); transition: height 0.5s; }
        .stats { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; }
        .stat { text-align: center; padding: 15px; background: #0f3460; border-radius: 8px; }
        .stat-label { font-size: 14px; color: #aaa; margin-bottom: 5px; }
        .stat-value { font-size: 24px; font-weight: bold; color: #4fc3f7; }
        .controls { display: flex; gap: 10px; justify-content: center; margin-top: 20px; }
        button { padding: 12px 24px; border: none; border-radius: 8px; font-size: 16px; cursor: pointer; transition: all 0.3s; }
        .btn-on { background: #4caf50; color: white; }
        .btn-off { background: #f44336; color: white; }
        .btn-auto { background: #2196f3; color: white; }
        button:hover { transform: translateY(-2px); box-shadow: 0 4px 8px rgba(0,0,0,0.3); }
    </style>
</head>
<body>
    <div class="container">
        <h1>Smart Water Pump</h1>
        
        <div class="card">
            <div class="level-display" id="level">--</div>
            <div class="tank">
                <div class="water" id="water" style="height: 0%"></div>
            </div>
        </div>
        
        <div class="card">
            <div class="stats">
                <div class="stat">
                    <div class="stat-label">Pump Status</div>
                    <div class="stat-value" id="pump-status">--</div>
                </div>
                <div class="stat">
                    <div class="stat-label">Mode</div>
                    <div class="stat-value" id="mode">--</div>
                </div>
                <div class="stat">
                    <div class="stat-label">Current Flow</div>
                    <div class="stat-value" id="flow">--</div>
                </div>
                <div class="stat">
                    <div class="stat-label">Daily Usage</div>
                    <div class="stat-value" id="daily">--</div>
                </div>
            </div>
            
            <div class="controls">
                <button class="btn-on" onclick="controlPump('on')">Turn ON</button>
                <button class="btn-off" onclick="controlPump('off')">Turn OFF</button>
                <button class="btn-auto" onclick="setMode('auto')">AUTO Mode</button>
            </div>
        </div>
    </div>
    
    <script>
        function updateData() {
            fetch('/api/telemetry')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('level').textContent = data.waterLevel.toFixed(1) + '%';
                    document.getElementById('water').style.height = data.waterLevel + '%';
                    document.getElementById('pump-status').textContent = data.motorState ? 'ON' : 'OFF';
                    document.getElementById('mode').textContent = data.mode;
                    document.getElementById('flow').textContent = data.currentInflow.toFixed(1) + ' cm3/s';
                    document.getElementById('daily').textContent = data.dailyUsage.toFixed(1) + ' L';
                })
                .catch(e => console.error('Update failed:', e));
        }
        
        function controlPump(action) {
            fetch('/api/pump/' + action, { method: 'POST' })
                .then(r => r.json())
                .then(data => {
                    if (data.success) updateData();
                    else alert(data.message || 'Failed');
                });
        }
        
        function setMode(mode) {
            fetch('/api/mode', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ mode: mode })
            })
                .then(r => r.json())
                .then(data => {
                    if (data.success) updateData();
                });
        }
        
        updateData();
        setInterval(updateData, 2000);
    </script>
</body>
</html>
)rawliteral";
    
    request->send(200, "text/html", html);
}

void WebServerLocal::handleStatus(AsyncWebServerRequest* request) {
    JsonDocument doc;
    doc["online"] = true;
    doc["firmware"] = FIRMWARE_VERSION;
    doc["uptime"] = millis() / 1000;
    
    String response;
    serializeJson(doc, response);
    
    AsyncWebServerResponse* resp = request->beginResponse(200, "application/json", response);
    addCORSHeaders(resp);
    request->send(resp);
}

void WebServerLocal::handleTelemetry(AsyncWebServerRequest* request) {
    JsonDocument doc;
    doc["waterLevel"] = _waterLevel;
    doc["currentInflow"] = _currentInflow;
    doc["maxInflow"] = _maxInflow;
    doc["motorState"] = _pump ? _pump->isOn() : false;
    doc["mode"] = _pump ? (_pump->getMode() == AUTO_MODE ? "AUTO" : 
                           _pump->getMode() == MANUAL_MODE ? "MANUAL" : "OVERRIDE") : "UNKNOWN";
    doc["dailyUsage"] = _tracker ? _tracker->getTodayUsage() : 0.0;
    doc["monthlyUsage"] = _tracker ? _tracker->getMonthUsage() : 0.0;
    doc["timestamp"] = millis();
    
    String response;
    serializeJson(doc, response);
    
    AsyncWebServerResponse* resp = request->beginResponse(200, "application/json", response);
    addCORSHeaders(resp);
    request->send(resp);
}

void WebServerLocal::handlePumpOn(AsyncWebServerRequest* request) {
    JsonDocument doc;
    
    if (_pump) {
        _pump->turnOn();
        doc["success"] = true;
        doc["message"] = "Pump turned on";
    } else {
        doc["success"] = false;
        doc["message"] = "Pump controller not available";
    }
    
    String response;
    serializeJson(doc, response);
    
    AsyncWebServerResponse* resp = request->beginResponse(200, "application/json", response);
    addCORSHeaders(resp);
    request->send(resp);
}

void WebServerLocal::handlePumpOff(AsyncWebServerRequest* request) {
    JsonDocument doc;
    
    if (_pump) {
        _pump->turnOff();
        doc["success"] = true;
        doc["message"] = "Pump turned off";
    } else {
        doc["success"] = false;
        doc["message"] = "Pump controller not available";
    }
    
    String response;
    serializeJson(doc, response);
    
    AsyncWebServerResponse* resp = request->beginResponse(200, "application/json", response);
    addCORSHeaders(resp);
    request->send(resp);
}

void WebServerLocal::handleSetMode(AsyncWebServerRequest* request) {
    // Handle body in the request
    request->send(200, "application/json", "{\"success\":true}");
}

void WebServerLocal::handleGetConfig(AsyncWebServerRequest* request) {
    if (!_storage) {
        request->send(500, "application/json", "{\"error\":\"Storage not available\"}");
        return;
    }
    
    TankConfig config = _storage->loadTankConfig();
    
    JsonDocument doc;
    doc["tankHeight"] = config.tankHeight;
    doc["tankLength"] = config.tankLength;
    doc["tankWidth"] = config.tankWidth;
    doc["tankRadius"] = config.tankRadius;
    doc["shape"] = (config.shape == RECTANGULAR) ? "rectangular" : "cylindrical";
    doc["upperThreshold"] = config.upperThreshold;
    doc["lowerThreshold"] = config.lowerThreshold;
    doc["maxInflow"] = config.maxInflow;
    
    String response;
    serializeJson(doc, response);
    
    AsyncWebServerResponse* resp = request->beginResponse(200, "application/json", response);
    addCORSHeaders(resp);
    request->send(resp);
}

void WebServerLocal::handleSetConfig(AsyncWebServerRequest* request) {
    request->send(200, "application/json", "{\"success\":true}");
}

void WebServerLocal::handleWiFiScan(AsyncWebServerRequest* request) {
    int n = WiFi.scanNetworks();
    
    JsonDocument doc;
    JsonArray networks = doc.createNestedArray("networks");
    
    for (int i = 0; i < n && i < 20; i++) {
        JsonObject net = networks.createNestedObject();
        net["ssid"] = WiFi.SSID(i);
        net["rssi"] = WiFi.RSSI(i);
        net["secured"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    }
    
    WiFi.scanDelete();
    
    String response;
    serializeJson(doc, response);
    
    AsyncWebServerResponse* resp = request->beginResponse(200, "application/json", response);
    addCORSHeaders(resp);
    request->send(resp);
}

void WebServerLocal::handleWiFiConnect(AsyncWebServerRequest* request) {
    request->send(200, "application/json", "{\"success\":true}");
}

void WebServerLocal::handleSetup(AsyncWebServerRequest* request) {
    request->send(200, "application/json", "{\"success\":true}");
}

void WebServerLocal::handleUsageStats(AsyncWebServerRequest* request) {
    if (!_tracker) {
        request->send(500, "application/json", "{\"error\":\"Tracker not available\"}");
        return;
    }
    
    JsonDocument doc;
    doc["dailyUsage"] = _tracker->getTodayUsage();
    doc["monthlyUsage"] = _tracker->getMonthUsage();
    doc["todayCycles"] = _tracker->getTodayCycles();
    
    String response;
    serializeJson(doc, response);
    
    AsyncWebServerResponse* resp = request->beginResponse(200, "application/json", response);
    addCORSHeaders(resp);
    request->send(resp);
}

bool WebServerLocal::checkAuth(AsyncWebServerRequest* request) {
    // Implement authentication if WEB_ENABLE_AUTH is true
    return true;
}

void WebServerLocal::addCORSHeaders(AsyncWebServerResponse* response) {
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
}