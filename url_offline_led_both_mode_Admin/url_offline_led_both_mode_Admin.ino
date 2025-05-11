#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager

// Wi-Fi credentials for the Access Point
const char* ssid_ap = "CTRLDevice-01";
const char* password_ap = "12345678";
IPAddress apIP(192, 168, 4, 1);

// LED pin definitions
const int led1 = 2; // GPIO2
const int led2 = 5; // GPIO5
const int led3 = 4; // GPIO4

ESP8266WebServer server(80);
DNSServer dnsServer;

void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html><html><head>
    <title>ESP LED Control</title>
    <style>
      body { font-family: Arial; text-align: center; margin-top: 40px; }
      button { padding: 10px 20px; margin: 10px; font-size: 16px; }
      .status { font-weight: bold; margin: 5px; }
    </style>
    <script>
      function toggle(led, state) {
        fetch(`/${led}/${state}`).then(() => update());
      }
      function update() {
        fetch('/status')
          .then(res => res.json())
          .then(data => {
            document.getElementById('s1').innerText = data.led1;
            document.getElementById('s2').innerText = data.led2;
            document.getElementById('s3').innerText = data.led3;
          });
      }
      setInterval(update, 2000);
      window.onload = update;
    </script>
    </head><body>
    <h1>ESP8266 LED Control</h1>
    <p><a href="/admin">Go to Admin Page</a></p>
    <p>LED 1 (GPIO2): <span class="status" id="s1">...</span></p>
    <button onclick="toggle('led1','on')">ON</button>
    <button onclick="toggle('led1','off')">OFF</button>
    <p>LED 2 (GPIO5): <span class="status" id="s2">...</span></p>
    <button onclick="toggle('led2','on')">ON</button>
    <button onclick="toggle('led2','off')">OFF</button>
    <p>LED 3 (GPIO4): <span class="status" id="s3">...</span></p>
    <button onclick="toggle('led3','on')">ON</button>
    <button onclick="toggle('led3','off')">OFF</button>
    </body></html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void handleLED(const int pin, bool state) {
  digitalWrite(pin, state ? LOW : HIGH);
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  String json = "{";
  json += "\"led1\":\"" + String(digitalRead(led1) == LOW ? "ON" : "OFF") + "\",";
  json += "\"led2\":\"" + String(digitalRead(led2) == LOW ? "ON" : "OFF") + "\",";
  json += "\"led3\":\"" + String(digitalRead(led3) == LOW ? "ON" : "OFF") + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleAdmin() {
  String html = R"rawliteral(
    <!DOCTYPE html><html><head>
    <title>ESP8266 Admin</title>
    <style>
      body { font-family: Arial; text-align: center; margin-top: 40px; }
      button { padding: 10px 20px; margin: 5px; font-size: 16px; }
      p { margin: 5px; }
    </style>
    <script>
      let connectionInterval; // To periodically check connection status

      function scanWiFi() {
        document.getElementById('wifiStatus').innerText = 'Scanning...';
        fetch('/scan')
          .then(response => response.text())
          .then(data => {
            document.getElementById('wifiList').innerHTML = data;
            document.getElementById('wifiStatus').innerText = '';
          });
      }
      function connectWiFi(ssid, password) {
        fetch(`/connect?ssid=${ssid}&password=${password}`)
          .then(response => response.text())
          .then(data => {
            document.getElementById('connectionStatus').innerText = data;
            clearInterval(connectionInterval);
            connectionInterval = setInterval(updateNetworkInfo, 2000);
          });
      }
      function showConnectDialog(ssid) {
        const password = prompt(`Enter password for ${ssid}:`);
        if (password !== null) {
          connectWiFi(ssid, password);
        }
      }
      function getNetworkInfo() {
        fetch('/networkinfo')
          .then(response => response.json())
          .then(data => {
            document.getElementById('staStatus').innerText = 'STA Status: ' + data.sta_status_text;
            document.getElementById('staIP').innerText = 'STA IP: ' + data.sta_ip;
            document.getElementById('apIP').innerText = 'AP IP: ' + data.ap_ip;
            document.getElementById('accessMethod').innerText = 'Access via: http://' + data.sta_ip + ' (Wi-Fi) or http://' + data.ap_ip + ' (Direct)';
          });
      }
      function updateNetworkInfo() {
        getNetworkInfo();
      }
      window.onload = getNetworkInfo;
      setInterval(updateNetworkInfo, 5000); // Update network info periodically
    </script>
    </head><body>
    <h1>ESP8266 Admin</h1>
    <p id="connectionStatus"></p>
    <p id="staStatus"></p>
    <p id="staIP"></p>
    <p id="apIP"></p>
    <p id="accessMethod"></p>
    <h2>Wi-Fi Management</h2>
    <p id="wifiStatus"></p>
    <button onclick="scanWiFi()">Scan for Wi-Fi Networks</button>
    <div id="wifiList"></div>
    <p><a href="/">Go to LED Control</a></p>
    <p><strong>Note:</strong> This ESP is running in dual Wi-Fi mode. You can connect to it directly via 'ESP-LED-AP' or through your connected Wi-Fi network.</p>
    </body></html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleScan() {
  String json = "<p>Scanning...</p>";
  WiFi.scanNetworks(true); // Start async scan

  int n = WiFi.scanComplete();
  if (n == -2) {
    json = "<p>Scan started...</p>";
  } else if (n == -1) {
    json = "<p>Scan failed.</p>";
  } else if (n == 0) {
    json = "<p>No networks found.</p>";
  } else {
    json = "<ul>";
    for (int i = 0; i < n; ++i) {
      json += "<li><strong>" + WiFi.SSID(i) + "</strong> (Signal: " + WiFi.RSSI(i) + " dBm) ";
      json += "<button onclick=\"showConnectDialog('" + WiFi.SSID(i) + "')\">Connect</button></li>";
    }
    json += "</ul>";
  }
  server.send(200, "text/html", json);
}

void handleConnect() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");

  if (ssid.length() > 0) {
    WiFi.begin(ssid.c_str(), password.c_str());
    server.send(200, "text/plain", "Attempting to connect to " + ssid + "...");
  } else {
    server.send(400, "text/plain", "SSID not provided.");
  }
}

String getWifiStatusString() {
  switch (WiFi.status()) {
    case WL_IDLE_STATUS: return "Idle";
    case WL_NO_SSID_AVAIL: return "SSID Not Available";
    case WL_SCAN_COMPLETED: return "Scan Completed";
    case WL_CONNECTED: return "Connected";
    case WL_CONNECT_FAILED: return "Connection Failed";
    case WL_DISCONNECTED: return "Disconnected";
    default: return "Unknown Status";
  }
}

void handleNetworkInfo() {
  String json = "{";
  json += "\"sta_status\":\"" + String(WiFi.status()) + "\",";
  json += "\"sta_status_text\":\"" + getWifiStatusString() + "\",";
  json += "\"sta_ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  // Set Wi-Fi mode to STA+AP for dual operation
  WiFi.mode(WIFI_AP_STA);

  // Configure Access Point
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid_ap, password_ap);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // DNS server for "ctrl.me" redirection in AP mode
  dnsServer.start(53, "ctrl.me", apIP);

  // Attempt to connect to saved Wi-Fi using WiFiManager
  WiFiManager wifiManager;
  wifiManager.setAPCallback([](WiFiManager* wm) {
    Serial.println("AP Mode active (Configuration Portal)");
  });

  // Reset saved settings if needed for testing
  // wifiManager.resetSettings();

  if (wifiManager.autoConnect("IOTconfig-01")) {
    Serial.println("Connected to Wi-Fi!");
    Serial.print("STA IP address: ");
    Serial.println(WiFi.localIP());
    // Start mDNS after successful Wi-Fi connection
    if (MDNS.begin("ctrl")) {
      Serial.println("mDNS responder started: http://ctrl.local");
    } else {
      Serial.println("Error starting mDNS");
    }
  } else {
    Serial.println("Configuration portal running on AP...");
    // AP is already running due to WiFi.mode(WIFI_AP_STA)
  }

  // Set pin modes
  pinMode(led1, OUTPUT); digitalWrite(led1, HIGH);
  pinMode(led2, OUTPUT); digitalWrite(led2, HIGH);
  pinMode(led3, OUTPUT); digitalWrite(led3, HIGH);

  // Web server routes
  server.on("/", handleRoot);
  server.on("/admin", handleAdmin);
  server.on("/scan", handleScan);
  server.on("/connect", handleConnect);
  server.on("/networkinfo", handleNetworkInfo);
  server.on("/led1/on", []() { handleLED(led1, true); });
  server.on("/led1/off", []() { handleLED(led1, false); });
  server.on("/led2/on", []() { handleLED(led2, true); });
  server.on("/led2/off", []() { handleLED(led2, false); });
  server.on("/led3/on", []() { handleLED(led3, true); });
  server.on("/led3/off", []() { handleLED(led3, false); });
  server.on("/status", handleStatus);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  dnsServer.processNextRequest(); // Important for AP mode + ctrl.me
  server.handleClient();
}