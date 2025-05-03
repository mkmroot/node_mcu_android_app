#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>

// Wi-Fi credentials
const char* ssid_ap = "ESP-LED-AP";
const char* password_ap = "12345678";

const char* ssid_sta = "YourHomeSSID";        // Replace with your Wi-Fi
const char* password_sta = "YourHomePassword";

// DNS & Web server setup
ESP8266WebServer server(80);
DNSServer dnsServer;
IPAddress apIP(192, 168, 4, 1);

// LED pin definitions
const int led1 = 2; // GPIO2
const int led2 = 5; // GPIO5
const int led3 = 4; // GPIO4

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

void setup() {
  Serial.begin(115200);

  // Set pins
  pinMode(led1, OUTPUT); digitalWrite(led1, HIGH);
  pinMode(led2, OUTPUT); digitalWrite(led2, HIGH);
  pinMode(led3, OUTPUT); digitalWrite(led3, HIGH);

  // Wi-Fi in dual mode
  WiFi.mode(WIFI_AP_STA);

  // Start AP mode
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid_ap, password_ap);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // DNS server for "ctrl.me" redirection in AP mode
  dnsServer.start(53, "ctrl.me", apIP);

  // Try connecting to home Wi-Fi
  WiFi.begin(ssid_sta, password_sta);
  Serial.print("Connecting to home Wi-Fi");
  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to home Wi-Fi!");
    Serial.print("STA IP: ");
    Serial.println(WiFi.localIP());

    // Start mDNS for ctrl.local
    if (MDNS.begin("ctrl")) {
      Serial.println("mDNS responder started: http://ctrl.local");
    } else {
      Serial.println("Error starting mDNS");
    }
  } else {
    Serial.println("\nFailed to connect to home Wi-Fi");
  }

  // Web server routes
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
  dnsServer.processNextRequest();  // Only affects AP mode + ctrl.me
  server.handleClient();
}
