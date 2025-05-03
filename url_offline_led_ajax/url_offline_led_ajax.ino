#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

const char* ssid = "ESP-LED-AP";
const char* password = "12345678";

const char* sta_ssid = "YourSSID";        // Your home/room Wi-Fi SSID
const char* sta_password = "YourPassword"; // Your home/room Wi-Fi password

ESP8266WebServer server(80);
DNSServer dnsServer;

const int led1 = 2; // GPIO2 (D4)
const int led2 = 5; // GPIO5 (D1)
const int led3 = 4; // GPIO4 (D2)

void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>ESP8266 LED Control</title>
      <style>
        body { font-family: Arial; text-align: center; margin-top: 50px; }
        button { padding: 10px 20px; margin: 10px; font-size: 16px; }
        .status { margin: 10px; font-weight: bold; }
      </style>
      <script>
        function toggleLED(led, action) {
          fetch(`/${led}/${action}`).then(() => updateStatus());
        }

        function updateStatus() {
          fetch('/status')
            .then(res => res.json())
            .then(data => {
              document.getElementById('status1').innerText = data.led1;
              document.getElementById('status2').innerText = data.led2;
              document.getElementById('status3').innerText = data.led3;
            });
        }

        setInterval(updateStatus, 2000);
        window.onload = updateStatus;
      </script>
    </head>
    <body>
      <h1>ESP8266 LED Control (Dual Mode)</h1>

      <div>
        <p>LED 1 (GPIO2) is: <span class="status" id="status1">...</span></p>
        <button onclick="toggleLED('led1', 'on')">Turn ON</button>
        <button onclick="toggleLED('led1', 'off')">Turn OFF</button>
      </div>

      <div>
        <p>LED 2 (GPIO5) is: <span class="status" id="status2">...</span></p>
        <button onclick="toggleLED('led2', 'on')">Turn ON</button>
        <button onclick="toggleLED('led2', 'off')">Turn OFF</button>
      </div>

      <div>
        <p>LED 3 (GPIO4) is: <span class="status" id="status3">...</span></p>
        <button onclick="toggleLED('led3', 'on')">Turn ON</button>
        <button onclick="toggleLED('led3', 'off')">Turn OFF</button>
      </div>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void handleLED(const int pin, bool state) {
  digitalWrite(pin, state ? LOW : HIGH);
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  String json = "{";
  json += "\"led1\":\"" + String((digitalRead(led1) == LOW) ? "ON" : "OFF") + "\",";
  json += "\"led2\":\"" + String((digitalRead(led2) == LOW) ? "ON" : "OFF") + "\",";
  json += "\"led3\":\"" + String((digitalRead(led3) == LOW) ? "ON" : "OFF") + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);

  // Set Wi-Fi mode to AP + STA
  WiFi.mode(WIFI_AP_STA);

  // Start AP mode
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Try connecting to home Wi-Fi
  WiFi.begin(sta_ssid, sta_password);
  Serial.print("Connecting to ");
  Serial.print(sta_ssid);
  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to home Wi-Fi");
    Serial.print("STA IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to home Wi-Fi.");
  }

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);

  digitalWrite(led1, HIGH);
  digitalWrite(led2, HIGH);
  digitalWrite(led3, HIGH);

  // Routes
  server.on("/", handleRoot);
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
  server.handleClient();
}
