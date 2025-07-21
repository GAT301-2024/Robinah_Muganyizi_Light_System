// Required Libraries for ESP32 Web Server
#include <WiFi.h> // For ESP32 Wi-Fi functionalities
#include <AsyncTCP.h> // Asynchronous TCP library, a dependency for ESPAsyncWebServer
#include <ESPAsyncWebServer.h> // Asynchronous Web Server library for ESP32

// --- Wi-Fi Configuration ---
// Define the SSID (network name) for your ESP32's Access Point
const char* ssid = "ROBINAH_tel";
// Define the password for your ESP32's Access Point. IMPORTANT: Change this to a strong, unique password!
const char* password = "g2i(1203"; // <--- CHANGE THIS PASSWORD!

// --- LED Pin Definitions (Based on Schematic) ---
// These GPIO pins control the BASE of the NPN transistors that switch the LEDs.
const int LED1_CTRL_PIN = 18;  // GPIO18 → Q1 base
const int LED2_CTRL_PIN = 19;  // GPIO19 → Q2 base
const int LED3_CTRL_PIN = 21;  // GPIO21 → Q3 base

// --- LDR Pin Definition (Based on Schematic) ---
const int LDR_PIN = 34; // Connected to GPIO 34 for analog reading

// --- Automatic Control Settings ---
// Threshold for determining night/day based on LDR reading.
// Values range from 0 (darkest) to 4095 (brightest) for a 12-bit ADC.
// You will likely need to CALIBRATE this value for your specific LDR and environment.
// Lower value = darker for "night" detection.
const int NIGHT_THRESHOLD = 800; // Example: Below 800 is considered night. ADJUST THIS!

// Delay between automatic light checks (in milliseconds)
const long AUTO_CHECK_INTERVAL = 10000; // Check every 10 seconds

// --- Web Server Object ---
// Create an instance of the AsyncWebServer on port 80 (standard HTTP port)
AsyncWebServer server(80);

// --- LED State Variables ---
// Boolean variables to keep track of the current state of each LED (true for ON, false for OFF)
bool led1State = false;
bool led2State = false;
bool led3State = false;

// --- Automatic Mode State Variable ---
bool autoModeEnabled = false;

// --- Timing Variable for Automatic Control ---
unsigned long lastAutoCheckMillis = 0;

// --- Helper function to set LED state (turns transistor ON/OFF) ---
void setLED(int pin, bool state) {
  // Since we are driving NPN transistors in common-emitter configuration:
  // HIGH on base turns transistor ON -> LED ON
  // LOW on base turns transistor OFF -> LED OFF
  digitalWrite(pin, state ? HIGH : LOW);
}

// --- HTML Content for the Web Dashboard ---
String getDashboardHtml() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>DENZO TECHNOLOGIES - TEAM_A Project</title>
    <link href="https://fonts.googleapis.com/css2?family=Poppins:wght@300;400;500;600&display=swap" rel="stylesheet">
    <style>
        :root {
            --primary: #5e35b1;
            --primary-light: #7e57c2;
            --secondary: #00acc1;
            --success: #43a047;
            --danger: #e53935;
            --light: #f5f5f5;
            --dark: #263238;
            --card-bg: #ffffff;
            --text-primary: #212121;
            --text-secondary: #757575;
        }
        
        body {
            font-family: 'Poppins', sans-serif;
            background: linear-gradient(135deg, #f5f7fa 0%, #e4e8f0 100%);
            margin: 0;
            padding: 20px;
            min-height: 100vh;
            color: var(--text-primary);
        }
        
        .container {
            max-width: 800px;
            margin: 0 auto;
        }
        
        .dashboard-card {
            background: var(--card-bg);
            border-radius: 16px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.1);
            overflow: hidden;
            margin-bottom: 20px;
        }
        
        .card-header {
            background: linear-gradient(135deg, var(--primary) 0%, var(--primary-light) 100%);
            color: white;
            padding: 20px;
            text-align: center;
        }
        
        .card-header h1 {
            margin: 0;
            font-size: 24px;
            font-weight: 600;
        }
        
        .card-header h2 {
            margin: 5px 0 0;
            font-size: 16px;
            font-weight: 400;
            opacity: 0.9;
        }
        
        .card-body {
            padding: 25px;
        }
        
        .info-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin-bottom: 25px;
        }
        
        .info-card {
            background: var(--light);
            border-radius: 10px;
            padding: 15px;
            box-shadow: 0 2px 5px rgba(0,0,0,0.05);
        }
        
        .info-label {
            font-size: 12px;
            color: var(--text-secondary);
            margin-bottom: 5px;
            font-weight: 500;
        }
        
        .info-value {
            font-size: 16px;
            font-weight: 600;
            color: var(--dark);
        }
        
        .wifi-card {
            display: flex;
            align-items: center;
            background: rgba(0, 172, 193, 0.1);
            border-radius: 10px;
            padding: 15px;
            margin-bottom: 25px;
        }
        
        .wifi-icon {
            font-size: 24px;
            color: var(--secondary);
            margin-right: 15px;
        }
        
        .wifi-details {
            flex: 1;
        }
        
        .wifi-name {
            font-weight: 600;
            margin-bottom: 3px;
        }
        
        .wifi-status {
            font-size: 13px;
            color: var(--text-secondary);
        }
        
        .control-panel {
            margin-bottom: 25px;
        }
        
        .panel-title {
            display: flex;
            align-items: center;
            margin-bottom: 15px;
            color: var(--primary);
            font-weight: 500;
        }
        
        .panel-title i {
            margin-right: 10px;
            font-size: 20px;
        }
        
        .btn-group {
            display: flex;
            flex-wrap: wrap;
            gap: 15px;
            justify-content: center;
        }
        
        .btn {
            flex: 1;
            min-width: 150px;
            max-width: 200px;
            background: var(--primary);
            color: white;
            border: none;
            border-radius: 8px;
            padding: 12px;
            font-size: 14px;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.3s ease;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        
        .btn i {
            margin-right: 8px;
        }
        
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(94, 53, 177, 0.3);
        }
        
        .btn:active {
            transform: translateY(0);
        }
        
        .btn.on {
            background: var(--success);
        }
        
        .status-indicator {
            display: inline-block;
            width: 12px;
            height: 12px;
            border-radius: 50%;
            margin-left: 8px;
        }
        
        .status-indicator.on {
            background: var(--success);
            box-shadow: 0 0 8px rgba(67, 160, 71, 0.6);
        }
        
        .status-indicator.off {
            background: var(--danger);
            box-shadow: 0 0 8px rgba(229, 57, 53, 0.6);
        }
        
        .sensor-info {
            text-align: center;
            margin-top: 15px;
            font-size: 14px;
            color: var(--text-secondary);
        }
        
        .sensor-info strong {
            color: var(--primary);
            font-weight: 500;
        }
        
        footer {
            text-align: center;
            margin-top: 30px;
            color: var(--text-secondary);
            font-size: 13px;
        }
        
        @media (max-width: 600px) {
            .btn-group {
                flex-direction: column;
                align-items: center;
            }
            
            .btn {
                width: 100%;
                max-width: none;
            }
        }
    </style>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css">
</head>
<body>
    <div class="container">
        <div class="dashboard-card">
            <div class="card-header">
                <h1>DENZO TECHNOLOGIES</h1>
                <h2>ESP32 Smart Lighting Control</h2>
            </div>
            
            <div class="card-body">
                <div class="info-grid">
                    <div class="info-card">
                        <div class="info-label">Student Name</div>
                        <div class="info-value" id="studentName">MUGANYIZI ROBINAH</div>
                    </div>
                    <div class="info-card">
                        <div class="info-label">Registration Number</div>
                        <div class="info-value" id="regNumber">24/U/4892/GIM</div>
                    </div>
                    <div class="info-card">
                        <div class="info-label">Project</div>
                        <div class="info-value">smart light control system</div>
                    </div>
                    <div class="info-card">
                        <div class="info-label">Date</div>
                        <div class="info-value" id="currentDate">Loading...</div>
                    </div>
                </div>
                
                <div class="wifi-card">
                    <div class="wifi-icon">
                        <i class="fas fa-wifi"></i>
                    </div>
                    <div class="wifi-details">
                        <div class="wifi-name" id="wifiSSID">Not connected</div>
                        <div class="wifi-status" id="wifiStatus">IP: Loading...</div>
                    </div>
                </div>
                
                <div class="control-panel">
                    <div class="panel-title">
                        <i class="fas fa-robot"></i> Automatic Mode
                    </div>
                    <div style="display: flex; justify-content: center;">
                        <button id="autoModeButton" class="btn" onclick="toggleAutoMode()">
                            <i class="fas fa-magic"></i> Toggle Auto Mode
                        </button>
                        <span id="autoModeStatus" class="status-indicator off"></span>
                    </div>
                    <div class="sensor-info">
                        Light Sensor: <strong id="ldrValue">---</strong> (Threshold: <span id="thresholdValue">800</span>)
                    </div>
                </div>
                
                <div class="control-panel">
                    <div class="panel-title">
                        <i class="fas fa-lightbulb"></i> Manual Control
                    </div>
                    <div class="btn-group">
                        <div style="text-align: center;">
                            <button id="led1Button" class="btn" onclick="toggleLED(1)">
                                <i class="fas fa-lightbulb"></i> LED 1
                            </button>
                            <span id="led1Status" class="status-indicator off"></span>
                        </div>
                        <div style="text-align: center;">
                            <button id="led2Button" class="btn" onclick="toggleLED(2)">
                                <i class="fas fa-lightbulb"></i> LED 2
                            </button>
                            <span id="led2Status" class="status-indicator off"></span>
                        </div>
                        <div style="text-align: center;">
                            <button id="led3Button" class="btn" onclick="toggleLED(3)">
                                <i class="fas fa-lightbulb"></i> LED 3
                            </button>
                            <span id="led3Status" class="status-indicator off"></span>
                        </div>
                    </div>
                </div>
            </div>
        </div>
        
        <footer>
            <p>© <span id="currentYear"></span> DENZO TECHNOLOGIES - TEAM_A Project</p>
        </footer>
    </div>

    <script>
        // Student information - UPDATED WITH NEW DETAILS
        document.getElementById('studentName').textContent = "MUGANYIZI ROBINAH";
        document.getElementById('regNumber').textContent = "24/U/4892/GIM";
        
        // Set current date and year
        const now = new Date();
        document.getElementById('currentDate').textContent = now.toLocaleDateString();
        document.getElementById('currentYear').textContent = now.getFullYear();
        
        // Function to update WiFi information
        function updateWifiInfo(ssid, ip) {
            const wifiSSID = document.getElementById('wifiSSID');
            const wifiStatus = document.getElementById('wifiStatus');
            
            if (ssid) {
                wifiSSID.textContent = ssid;
                wifiStatus.textContent = `IP: ${ip || 'Not available'}`;
            } else {
                wifiSSID.textContent = "Not connected";
                wifiStatus.textContent = "Connect to ESP32 AP";
            }
        }
        
        // Function to send a request to the ESP32 to toggle an LED
        async function toggleLED(ledNum) {
            const button = document.getElementById(`led${ledNum}Button`);
            const statusIndicator = document.getElementById(`led${ledNum}Status`);

            try {
                const response = await fetch(`/led${ledNum}/toggle`);
                const data = await response.text();
                console.log(`Response for LED${ledNum}:`, data);
                updateUI(ledNum, data.includes("ON"));
            } catch (error) {
                console.error('Error toggling LED:', error);
                alert('Connection error. Please check your WiFi connection.');
            }
        }

        // Function to toggle Automatic Mode
        async function toggleAutoMode() {
            const button = document.getElementById('autoModeButton');
            const statusIndicator = document.getElementById('autoModeStatus');

            try {
                const response = await fetch('/automode/toggle');
                const data = await response.json();
                console.log('Auto Mode Toggled:', data);
                updateAutoModeUI(data.autoModeEnabled);
            } catch (error) {
                console.error('Error toggling Auto Mode:', error);
                alert('Connection error. Please check your WiFi connection.');
            }
        }

        // Function to fetch the current status from ESP32
        async function updateAllStatus() {
            try {
                const response = await fetch('/status');
                const data = await response.json();
                console.log("Current System Status:", data);

                updateUI(1, data.led1);
                updateUI(2, data.led2);
                updateUI(3, data.led3);
                updateAutoModeUI(data.autoModeEnabled);
                document.getElementById('ldrValue').textContent = data.ldrValue;
                
                // Update WiFi info if available in response
                if (data.wifiSSID) {
                    updateWifiInfo(data.wifiSSID, data.ipAddress);
                }

            } catch (error) {
                console.error('Error fetching system status:', error);
                updateWifiInfo(null);
            }
        }

        // Helper function to update LED UI
        function updateUI(ledNum, state) {
            const button = document.getElementById(`led${ledNum}Button`);
            const statusIndicator = document.getElementById(`led${ledNum}Status`);

            if (state) {
                button.classList.add('on');
                statusIndicator.className = 'status-indicator on';
            } else {
                button.classList.remove('on');
                statusIndicator.className = 'status-indicator off';
            }
        }

        // Helper function to update Auto Mode UI
        function updateAutoModeUI(state) {
            const button = document.getElementById('autoModeButton');
            const statusIndicator = document.getElementById('autoModeStatus');

            if (state) {
                button.classList.add('on');
                button.innerHTML = '<i class="fas fa-magic"></i> Auto Mode: ON';
                statusIndicator.className = 'status-indicator on';
            } else {
                button.classList.remove('on');
                button.innerHTML = '<i class="fas fa-magic"></i> Auto Mode: OFF';
                statusIndicator.className = 'status-indicator off';
            }
        }

        // Initialize on load
        window.onload = function() {
            updateAllStatus();
            setInterval(updateAllStatus, 3000); // Refresh every 3 seconds
        };
    </script>
</body>
</html>
)rawliteral";
  return html;
}

// --- Arduino Setup Function ---
void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting ESP32 Intelligent Lighting System...");

  // Set LED control pins as OUTPUTs
  pinMode(LED1_CTRL_PIN, OUTPUT);
  pinMode(LED2_CTRL_PIN, OUTPUT);
  pinMode(LED3_CTRL_PIN, OUTPUT);
  
  // Set LDR pin as INPUT (implicitly done for analogRead, but good practice)
  pinMode(LDR_PIN, INPUT);

  // Initialize all LEDs to OFF state
  setLED(LED1_CTRL_PIN, LOW);
  setLED(LED2_CTRL_PIN, LOW);
  setLED(LED3_CTRL_PIN, LOW);
  led1State = false;
  led2State = false;
  led3State = false;

  // Start the ESP32 in Access Point (AP) mode
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point (AP) IP address: ");
  Serial.println(IP);
  Serial.print("Connect to Wi-Fi network: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.println("Then open a web browser and go to the IP address above.");

  // --- Web Server Route Handlers ---

  // Route for the root URL ("/") - serves the main HTML dashboard
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Client requested root URL '/'");
    request->send(200, "text/html", getDashboardHtml());
  });

  // Route to toggle LED 1
  server.on("/led1/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    led1State = !led1State;
    setLED(LED1_CTRL_PIN, led1State);
    Serial.printf("LED 1 toggled to: %s\n", led1State ? "ON" : "OFF");
    request->send(200, "text/plain", led1State ? "LED1_ON" : "LED1_OFF");
  });

  // Route to toggle LED 2
  server.on("/led2/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    led2State = !led2State;
    setLED(LED2_CTRL_PIN, led2State);
    Serial.printf("LED 2 toggled to: %s\n", led2State ? "ON" : "OFF");
    request->send(200, "text/plain", led2State ? "LED2_ON" : "LED2_OFF");
  });

  // Route to toggle LED 3
  server.on("/led3/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    led3State = !led3State;
    setLED(LED3_CTRL_PIN, led3State);
    Serial.printf("LED 3 toggled to: %s\n", led3State ? "ON" : "OFF");
    request->send(200, "text/plain", led3State ? "LED3_ON" : "LED3_OFF");
  });

  // Route to toggle Automatic Mode
  server.on("/automode/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    autoModeEnabled = !autoModeEnabled;
    Serial.printf("Automatic Mode toggled to: %s\n", autoModeEnabled ? "ENABLED" : "DISABLED");
    String jsonResponse = "{ \"autoModeEnabled\": " + String(autoModeEnabled ? "true" : "false") + " }";
    request->send(200, "application/json", jsonResponse);
  });

  // Route to get the current status of all LEDs, Auto Mode, and LDR value as JSON
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    // Serial.println("Client requested system status '/status'"); // Uncomment for more verbose logging
    int ldrValue = analogRead(LDR_PIN); // Read LDR value
    
    String jsonResponse = "{";
    jsonResponse += "\"led1\":" + String(led1State ? "true" : "false") + ",";
    jsonResponse += "\"led2\":" + String(led2State ? "true" : "false") + ",";
    jsonResponse += "\"led3\":" + String(led3State ? "true" : "false") + ",";
    jsonResponse += "\"autoModeEnabled\":" + String(autoModeEnabled ? "true" : "false") + ",";
    jsonResponse += "\"ldrValue\":" + String(ldrValue);
    jsonResponse += "}";
    request->send(200, "application/json", jsonResponse);
  });

  // Start the web server
  server.begin();
  Serial.println("Web server started.");
}

// --- Arduino Loop Function ---
// This function runs repeatedly after setup()
void loop() {
  // Check for automatic light control if enabled
  if (autoModeEnabled) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastAutoCheckMillis >= AUTO_CHECK_INTERVAL) {
      lastAutoCheckMillis = currentMillis;

      int ldrValue = analogRead(LDR_PIN);
      Serial.printf("LDR Value: %d, Threshold: %d\n", ldrValue, NIGHT_THRESHOLD);

      if (ldrValue < NIGHT_THRESHOLD) { // It's dark (LDR value is low)
        Serial.println("It's NIGHT - Turning ALL LEDs ON automatically.");
        if (!led1State) { led1State = true; setLED(LED1_CTRL_PIN, HIGH); }
        if (!led2State) { led2State = true; setLED(LED2_CTRL_PIN, HIGH); }
        if (!led3State) { led3State = true; setLED(LED3_CTRL_PIN, HIGH); }
      } else { // It's bright (LDR value is high)
        Serial.println("It's DAY - Turning ALL LEDs OFF automatically.");
        if (led1State) { led1State = false; setLED(LED1_CTRL_PIN, LOW); }
        if (led2State) { led2State = false; setLED(LED2_CTRL_PIN, LOW); }
        if (led3State) { led3State = false; setLED(LED3_CTRL_PIN, LOW); }
      }
    }
  }
}
