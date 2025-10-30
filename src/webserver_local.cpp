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
    // Check if WiFi is available (either connected to network OR in AP mode)
    bool isAPMode = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA);
    if (WiFi.status() != WL_CONNECTED && !isAPMode) {
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
    Serial.println("=================================");
    Serial.println("WEB SERVER STARTED");
    Serial.print("Access at: http://");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print(WiFi.localIP());
    } else {
        Serial.print(WiFi.softAPIP());
    }
    Serial.print(":");
    Serial.println(WEBSERVER_PORT);
    Serial.println("=================================");
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

    // Setup endpoint with body parser
    _server->on("/api/setup", HTTP_POST,
        [this](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            handleSetup(request, data, len, index, total);
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
    // Check if we're in first-time setup mode
    if (_storage && _storage->isFirstTimeSetup()) {
        // Serve setup page
        const char setupHtml[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Smart Water Pump - Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: Arial, sans-serif; background: #1a1a2e; color: #eee; padding: 20px; }
        .container { max-width: 600px; margin: 0 auto; }
        h1 { text-align: center; margin-bottom: 10px; color: #4fc3f7; }
        .subtitle { text-align: center; color: #aaa; margin-bottom: 30px; }
        .card { background: #16213e; padding: 25px; border-radius: 10px; margin-bottom: 20px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); }
        .section-title { color: #4fc3f7; margin-bottom: 15px; font-size: 18px; border-bottom: 2px solid #0f3460; padding-bottom: 8px; }
        label { display: block; margin-top: 15px; margin-bottom: 5px; color: #aaa; font-size: 14px; }
        input, select { width: 100%; padding: 10px; border: 1px solid #0f3460; border-radius: 5px; background: #0f3460; color: #eee; font-size: 14px; }
        input:focus, select:focus { outline: none; border-color: #4fc3f7; }
        .form-row { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; }
        button { width: 100%; padding: 15px; border: none; border-radius: 8px; font-size: 16px; font-weight: bold; cursor: pointer; margin-top: 20px; transition: all 0.3s; }
        .btn-submit { background: #4caf50; color: white; }
        .btn-submit:hover { background: #45a049; transform: translateY(-2px); box-shadow: 0 4px 8px rgba(0,0,0,0.3); }
        .btn-submit:disabled { background: #666; cursor: not-allowed; }
        .message { padding: 12px; border-radius: 5px; margin-top: 15px; display: none; }
        .error { background: #f44336; color: white; display: block; }
        .success { background: #4caf50; color: white; display: block; }
        .info { background: #2196f3; color: white; padding: 12px; border-radius: 5px; margin-bottom: 20px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🚰 Smart Water Pump</h1>
        <div class="subtitle">First Time Setup</div>
        <div class="info">Configure your water tank and WiFi settings to get started</div>

        <form id="setupForm">
            <div class="card">
                <div class="section-title">Tank Configuration</div>
                <label>Tank Shape</label>
                <select id="shape" onchange="toggleShapeFields()">
                    <option value="rectangular">Rectangular</option>
                    <option value="cylindrical">Cylindrical</option>
                </select>

                <label>Tank Height (cm) *</label>
                <input type="number" id="height" step="0.1" min="10" max="1000" required>

                <div id="rectangularFields">
                    <div class="form-row">
                        <div>
                            <label>Length (cm) *</label>
                            <input type="number" id="length" step="0.1" min="10" max="1000">
                        </div>
                        <div>
                            <label>Width (cm) *</label>
                            <input type="number" id="width" step="0.1" min="10" max="1000">
                        </div>
                    </div>
                </div>

                <div id="cylindricalFields" style="display:none;">
                    <label>Radius (cm) *</label>
                    <input type="number" id="radius" step="0.1" min="5" max="500">
                </div>

                <div class="form-row">
                    <div>
                        <label>Lower Threshold (%)</label>
                        <input type="number" id="lowerThreshold" value="20" min="0" max="100" step="1">
                    </div>
                    <div>
                        <label>Upper Threshold (%)</label>
                        <input type="number" id="upperThreshold" value="90" min="0" max="100" step="1">
                    </div>
                </div>
            </div>

            <div class="card">
                <div class="section-title">WiFi Configuration (Optional)</div>
                <label>WiFi SSID</label>
                <input type="text" id="ssid" placeholder="Leave empty to skip WiFi setup">

                <label>WiFi Password</label>
                <input type="password" id="password" placeholder="WiFi password">
            </div>

            <button type="submit" class="btn-submit" id="submitBtn">Complete Setup</button>
            <div id="message" class="message"></div>
        </form>
    </div>

    <script>
        function toggleShapeFields() {
            const shape = document.getElementById('shape').value;
            document.getElementById('rectangularFields').style.display = shape === 'rectangular' ? 'block' : 'none';
            document.getElementById('cylindricalFields').style.display = shape === 'cylindrical' ? 'block' : 'none';
        }

        document.getElementById('setupForm').addEventListener('submit', async (e) => {
            e.preventDefault();
            const btn = document.getElementById('submitBtn');
            const msg = document.getElementById('message');

            btn.disabled = true;
            btn.textContent = 'Saving...';
            msg.style.display = 'none';

            const shape = document.getElementById('shape').value;
            const data = {
                tankHeight: parseFloat(document.getElementById('height').value),
                tankShape: shape,
                upperThreshold: parseFloat(document.getElementById('upperThreshold').value),
                lowerThreshold: parseFloat(document.getElementById('lowerThreshold').value),
                ssid: document.getElementById('ssid').value,
                password: document.getElementById('password').value
            };

            if (shape === 'rectangular') {
                data.tankLength = parseFloat(document.getElementById('length').value);
                data.tankWidth = parseFloat(document.getElementById('width').value);
            } else {
                data.tankRadius = parseFloat(document.getElementById('radius').value);
            }

            try {
                const response = await fetch('/api/setup', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(data)
                });

                const result = await response.json();

                if (result.success) {
                    msg.className = 'message success';
                    msg.textContent = 'Setup complete! Restarting...';
                    msg.style.display = 'block';
                    setTimeout(() => window.location.reload(), 3000);
                } else {
                    msg.className = 'message error';
                    msg.textContent = result.message || 'Setup failed';
                    msg.style.display = 'block';
                    btn.disabled = false;
                    btn.textContent = 'Complete Setup';
                }
            } catch (error) {
                msg.className = 'message error';
                msg.textContent = 'Error: ' + error.message;
                msg.style.display = 'block';
                btn.disabled = false;
                btn.textContent = 'Complete Setup';
            }
        });
    </script>
</body>
</html>
)rawliteral";
        request->send(200, "text/html", setupHtml);
    } else {
        // Serve dashboard page
        const char dashboardHtml[] = R"rawliteral(
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
        request->send(200, "text/html", dashboardHtml);
    }
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

void WebServerLocal::handleSetup(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    static String jsonBuffer;

    // Accumulate the data
    if (index == 0) {
        jsonBuffer = "";
    }

    for (size_t i = 0; i < len; i++) {
        jsonBuffer += (char)data[i];
    }

    // Only process when all data is received
    if (index + len != total) {
        return;
    }

    #if ENABLE_SERIAL_DEBUG
    Serial.println("Processing setup request...");
    Serial.println("Data: " + jsonBuffer);
    #endif

    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonBuffer);

    if (error) {
        #if ENABLE_SERIAL_DEBUG
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
        #endif
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
        jsonBuffer = "";
        return;
    }

    // Validate required fields
    if (!doc.containsKey("tankHeight") || !doc.containsKey("tankShape")) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing required fields\"}");
        jsonBuffer = "";
        return;
    }

    // Load current config and update it
    TankConfig config = _storage->loadTankConfig();

    config.tankHeight = doc["tankHeight"];
    config.upperThreshold = doc.containsKey("upperThreshold") ? (float)doc["upperThreshold"] : DEFAULT_UPPER_THRESHOLD;
    config.lowerThreshold = doc.containsKey("lowerThreshold") ? (float)doc["lowerThreshold"] : DEFAULT_LOWER_THRESHOLD;

    String shape = doc["tankShape"].as<String>();
    if (shape == "rectangular") {
        config.shape = RECTANGULAR;
        if (!doc.containsKey("tankLength") || !doc.containsKey("tankWidth")) {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing tank dimensions\"}");
            jsonBuffer = "";
            return;
        }
        config.tankLength = doc["tankLength"];
        config.tankWidth = doc["tankWidth"];
        config.tankRadius = 0.0;
    } else if (shape == "cylindrical") {
        config.shape = CYLINDRICAL;
        if (!doc.containsKey("tankRadius")) {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing tank radius\"}");
            jsonBuffer = "";
            return;
        }
        config.tankRadius = doc["tankRadius"];
        config.tankLength = 0.0;
        config.tankWidth = 0.0;
    }

    // Mark setup as complete
    config.firstTimeSetup = false;

    // Save configuration
    if (!_storage->saveTankConfig(config)) {
        request->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to save configuration\"}");
        jsonBuffer = "";
        return;
    }

    // Save WiFi credentials if provided
    if (doc.containsKey("ssid") && doc["ssid"].as<String>().length() > 0) {
        String ssid = doc["ssid"];
        String password = doc.containsKey("password") ? doc["password"].as<String>() : "";
        _storage->saveWiFiCredentials(ssid, password);

        #if ENABLE_SERIAL_DEBUG
        Serial.println("WiFi credentials saved: " + ssid);
        #endif
    }

    #if ENABLE_SERIAL_DEBUG
    Serial.println("Setup completed successfully!");
    #endif

    // Send success response
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Setup complete\"}");

    jsonBuffer = "";
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