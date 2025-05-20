// web_interface.h
// Web interface module for desk control

#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include "config.h"
#include "hall_sensor.h"
#include "motor_control.h"
#include "storage.h"

// Web server instance
WebServer server(80);



// Process web requests
void handleWebRequests() {
  server.handleClient();
}

// — HTTP auth check —
bool isAuthorized() {
  if (!server.hasArg("secret") || server.arg("secret") != SECRET_KEY) {
    server.send(401, "text/plain", "Unauthorized");
    return false;
  }
  return true;
}

// Return current height in cm based on pulse count
void handleHeight() {
  if (!isAuthorized()) return;
  float height = pulsesToHeight(pulseCount);
  server.send(200, "text/plain", String(height, 1));
}

// Return moving status: "1" if moving, "0" otherwise
void handleStatus() {
  if (!isAuthorized()) return;
  server.send(200, "text/plain", isMoving ? "1" : "0");
}

// Return Hall sensor info
void handleHallStatus() {
  if (!isAuthorized()) return;
  float height = pulsesToHeight(pulseCount);
  String response = "Height: " + String(height, 1) + " cm" +
                    ", Pulses: " + String(pulseCount) +
                    ", Direction: " + hallDirection;
  server.send(200, "text/plain", response);
}

// Reset pulse counter
void handleResetPulses() {
  if (!isAuthorized()) return;
  pulseCount = 0;
  lastPulseCount = 0;
  lastStoppedPulses = 0;
  savePulseCount();  // Save the reset position to EEPROM
  server.send(200, "text/plain", "Pulse counter reset to 0 and saved");
}

// Start non-blocking move to target height
void handleMoveToTarget() {
  if (!isAuthorized()) return;
  if (!server.hasArg("target")) { server.send(400,"text/plain","Missing target"); return; }
  float targetHeight = server.arg("target").toFloat();
  if (targetHeight < MIN_HEIGHT_CM || targetHeight > MAX_HEIGHT_CM) {
    server.send(400,"text/plain","Out of bounds"); return;
  }
  moveTargetPulses = heightToPulses(targetHeight);
  moveStartTime = millis();
  abortMovement = false;
  isMoving = true;
  
  // Decide direction based on pulse count
  if (moveTargetPulses > pulseCount) moveDeskUp(); else moveDeskDown();
  server.send(200,"text/plain","Movement started");
}

// Manual up/down
void handleManual() {
  if (!isAuthorized()) return;

  // Turn on backlight with any manual control
  if (!backlightOn) {
    lcd.backlight();
    backlightOn = true;
    lastMotorStopTime = millis();  // Reset timeout
  }

  if (!server.hasArg("dir")) { server.send(400,"text/plain","Missing dir"); return; }
  String d = server.arg("dir");
  if (d=="up") moveDeskUp();
  else if (d=="down") moveDeskDown();
  else { server.send(400,"text/plain","Invalid dir"); return; }
  server.send(200,"text/plain","Manual move " + d);
}

// Stop any motion
void handleManualStop() {
  if (!isAuthorized()) return;
  abortMovement = true;
  isMoving = false;
  stopDeskMotor();
  float height = pulsesToHeight(pulseCount);
  server.send(200,"text/plain","Stopped at " + String(height, 1) + " cm");
}

// Web UI
void handleWebPage() {
  // if missing or wrong secret, force login
  if (!server.hasArg("secret") || server.arg("secret") != SECRET_KEY) {
    server.sendHeader("Location", "/login");
    server.send(302, "text/html", "");
    return;
  }

  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head><meta name="viewport" content="width=device-width,initial-scale=1"><title>Desk Control</title>
<style>
  body{font-family:Arial,sans-serif;background:#eef2f5;color:#333}
  .container{max-width:360px;margin:40px auto;background:#fff;padding:20px;border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,0.1)}
  h1{text-align:center}
  .slider-container{margin:10px 0}
  input[type=range] { width:100%; }
  .status{margin:10px 0;text-align:center;font-size:1.2em}
  .btn{width:100%;padding:10px;margin:5px 0;border:none;border-radius:4px;cursor:pointer}
  .btn {
  user-select: none;
  -webkit-user-select: none;
  }
  .btn-primary{background:#007BFF;color:#fff}
  .btn-secondary{background:#6c757d;color:#fff}
  .btn-warning{background:#ffc107;color:#000}
  .controls{display:flex;justify-content:space-between}
  .controls .btn{width:48%}
  .sensor-data{margin-top:20px;font-size:0.9em;border-top:1px solid #ddd;padding-top:10px}
  .preset-buttons{margin-top:20px}
  .preset-row{display:flex;justify-content:space-between;margin-bottom:5px}
  .preset-row .btn{width:32%}
</style>
</head>
<body>
<div class="container">
  <h1>Desk Control</h1>
  <div class="status">Height: <span id="height">--</span> cm</div>

  <!-- Slider for target height -->
  <div class="slider-container">
    <label for="targetSlider">Target Height: <span id="targetVal">69.0</span> cm</label>
    <input type="range" id="targetSlider" min="69" max="105" step="0.5" value="69"
           oninput="updateSliderVal()" />
  </div>
  <button class="btn btn-primary" onclick="setHeight()">Set Height</button>

  <!-- Preset positions -->
  <div class="preset-buttons">
    <h3>Preset Heights</h3>
    <div class="preset-row">
      <button class="btn btn-secondary" onclick="goToPreset(69)">Lowest (67cm)</button>
      <button class="btn btn-secondary" onclick="goToPreset(77)">Low (77cm)</button>
      <button class="btn btn-secondary" onclick="goToPreset(87)">Mid (87cm)</button>
    </div>
    <div class="preset-row">
      <button class="btn btn-secondary" onclick="goToPreset(97)">High (97cm)</button>
      <button class="btn btn-secondary" onclick="goToPreset(102)">Higher (102cm)</button>
      <button class="btn btn-secondary" onclick="goToPreset(105)">Highest (107cm)</button>
    </div>
  </div>

  <div class="controls">
    <button class="btn btn-primary"
            onpointerdown="startMove('up')" onpointerup="stopMove()" onpointercancel="stopMove()"
    >Up</button>
    <button class="btn btn-primary"
            onpointerdown="startMove('down')" onpointerup="stopMove()" onpointercancel="stopMove()"
    >Down</button>
  </div>

  <button class="btn btn-secondary" onclick="stopMove()">Stop</button>
  <button class="btn btn-warning" onclick="resetPulses()">Reset to Zero</button>
  <div id="status" class="status"></div>
  
  <!-- Hall Sensor Data -->
  <div class="sensor-data">
    <h3>Sensor Data</h3>
    <div id="hallData">Loading...</div>
  </div>
</div>
<script>
  const params = new URLSearchParams(window.location.search);
  const secret = params.get('secret');
  if (!secret) { document.body.innerHTML = '<p style="text-align:center;">Unauthorized</p>'; throw ''; }

  function refreshHeight() {
    fetch(`/height?secret=${secret}`)
      .then(r => r.text())
      .then(t => { document.getElementById('height').innerText = t; });
  }
  
  function refreshHallData() {
    fetch(`/hall?secret=${secret}`)
      .then(r => r.text())
      .then(t => { document.getElementById('hallData').innerText = t; });
  }
  
  function resetPulses() {
    if(!confirm('Reset position to 0? Make sure desk is at lowest position (67cm)!')) return;
    fetch(`/reset-pulses?secret=${secret}`)
      .then(r => r.text())
      .then(t => { 
        document.getElementById('status').innerText = t;
        refreshHallData();
        refreshHeight();
        document.getElementById('targetSlider').value = 69;
        updateSliderVal();
      });
  }

  function updateSliderVal() {
    const v = parseFloat(document.getElementById('targetSlider').value).toFixed(1);
    document.getElementById('targetVal').innerText = v;
  }

  function setHeight() {
    const val = document.getElementById('targetSlider').value;
    document.getElementById('status').innerText = 'Moving to ' + val + ' cm';
    fetch(`/move?target=${val}&secret=${secret}`)
      .then(r => r.text())
      .then(_ => {
        const poll = setInterval(() => {
          fetch(`/status?secret=${secret}`)
            .then(r => r.text())
            .then(s => {
              refreshHallData(); // Update sensor data while moving
              if (s === '0') {
                clearInterval(poll);
                refreshHeight();
                document.getElementById('status').innerText =
                  'Stopped at ' + document.getElementById('height').innerText + ' cm';
              }
            });
        }, 500);
      });
  }
  
  function goToPreset(height) {
    document.getElementById('targetSlider').value = height;
    updateSliderVal();
    setHeight();
  }

  function startMove(dir) { fetch(`/manual?dir=${dir}&secret=${secret}`); }
  function stopMove() {
    fetch(`/stop?secret=${secret}`)
      .then(r => r.text())
      .then(msg => {
        document.getElementById('status').innerText = msg;
        refreshHeight();
        refreshHallData();
      });
  }

  document.addEventListener('DOMContentLoaded', () => {
    refreshHeight();
    updateSliderVal();
    refreshHallData();
    
    // Periodically update sensor data
    setInterval(refreshHallData, 1000);
  });
</script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

// Show login form
void handleLoginPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Sign In</title>
  <style>
    body{font-family:Arial,sans-serif;background:#eef2f5;color:#333}
    .container{max-width:360px;margin:80px auto;background:#fff;padding:20px;
               border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,0.1)}
    input{width:100%;padding:10px;margin:8px 0;border:1px solid #ccc;
          border-radius:4px;font-size:1em}
    .btn{width:100%;padding:10px;border:none;border-radius:4px;
         background:#007BFF;color:#fff;font-size:1em;cursor:pointer}
  </style>
</head>
<body>
  <div class="container">
    <h2>Sign In</h2>
    <form method="POST" action="/login">
      <input type="email"   name="user" placeholder="Email"    required/>
      <input type="password" name="pass" placeholder="Password" required/>
      <button class="btn" type="submit">Sign In</button>
    </form>
  </div>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

// Process login
void handleLogin() {
  if (!server.hasArg("user") || !server.hasArg("pass")) {
    server.send(400, "text/plain", "Missing credentials");
    return;
  }
  String u = server.arg("user"), p = server.arg("pass");
  if (u == AUTH_USER && p == AUTH_PASS) {
    // redirect to main with secret
    server.sendHeader("Location", String("/?secret=") + SECRET_KEY);
    server.send(302, "text/plain", "");
  } else {
    server.send(401, "text/html",
      "<p>Invalid credentials</p><a href=\"/login\">Try again</a>");
  }
}

// Initialize web server
void initWebServer() {
  // Wi-Fi setup
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println("\nConnected! IP = " + WiFi.localIP().toString());
  
  // mDNS setup
  if (MDNS.begin("deskcontrol")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS started (http://deskcontrol.local)");
  }

  // Setup route handlers
  server.on("/login", HTTP_GET, handleLoginPage);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/", handleWebPage);
  server.on("/height", handleHeight);
  server.on("/move", handleMoveToTarget);
  server.on("/manual", handleManual);
  server.on("/stop", handleManualStop);
  server.on("/status", handleStatus);
  server.on("/hall", handleHallStatus);
  server.on("/reset-pulses", handleResetPulses);
  
  // Start web server
  server.begin();
  Serial.println("Web server started");
}

#endif // WEB_INTERFACE_H