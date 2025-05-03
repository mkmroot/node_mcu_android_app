#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

const char* ssid = "MyESP8266AP";
const char* password = "mypassword";

ESP8266WebServer server(80);
DNSServer dnsServer;

const int ledPin = 2; // D4
const int ledPin2 = 5; // D1
const int ledPin3 = 4; // D2

IPAddress localIP(192, 168, 4, 1);

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
          fetch(`/${led}/${action}`)
            .then(() => updateStatus());
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
      <h1>ESP8266 LED Control (AJAX)</h1>

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

// LED Control Endpoints
void handleLED(const int pin, bool state) {
  digitalWrite(pin, state ? LOW : HIGH);
  server.send(200, "text/plain", "OK");
}

// JSON Status Endpoint
void handleStatus() {
  String json = "{";
  json += "\"led1\":\"" + String((digitalRead(ledPin) == LOW) ? "ON" : "OFF") + "\",";
  json += "\"led2\":\"" + String((digitalRead(ledPin2) == LOW) ? "ON" : "OFF") + "\",";
  json += "\"led3\":\"" + String((digitalRead(ledPin3) == LOW) ? "ON" : "OFF") + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(ledPin3, OUTPUT);

  digitalWrite(ledPin, HIGH);
  digitalWrite(ledPin2, HIGH);
  digitalWrite(ledPin3, HIGH);

  Serial.begin(115200);

  WiFi.softAP(ssid, password);
  dnsServer.start(53, "ctrl.me", localIP);

  server.on("/", handleRoot);

  server.on("/led1/on", []() { handleLED(ledPin, true); });
  server.on("/led1/off", []() { handleLED(ledPin, false); });
  server.on("/led2/on", []() { handleLED(ledPin2, true); });
  server.on("/led2/off", []() { handleLED(ledPin2, false); });
  server.on("/led3/on", []() { handleLED(ledPin3, true); });
  server.on("/led3/off", []() { handleLED(ledPin3, false); });

  server.on("/status", handleStatus);

  server.onNotFound([]() {
    server.send(404, "text/plain", "404: Not Found");
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}
