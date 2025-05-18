#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <EEPROM.h>

const char* ssid_ap = "CTRLDevice-01";
const char* password_ap = "12345678";
IPAddress apIP(192, 168, 4, 1);

const int led1 = 2;
const int led2 = 5;
const int led3 = 4;
const uint16_t RECV_PIN = 13;

uint32_t LED1_ON_CODE;
uint32_t LED1_OFF_CODE;
uint32_t LED2_ON_CODE;
uint32_t LED2_OFF_CODE;
uint32_t LED3_ON_CODE;
uint32_t LED3_OFF_CODE;

uint32_t irCodes[5] = {0};  // last 5 received IR codes

ESP8266WebServer server(80);
DNSServer dnsServer;
IRrecv irrecv(RECV_PIN);
decode_results results;

#define EEPROM_SIZE 24  // 6 IR codes * 4 bytes each

void loadIRCodes() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, LED1_ON_CODE);
  EEPROM.get(4, LED1_OFF_CODE);
  EEPROM.get(8, LED2_ON_CODE);
  EEPROM.get(12, LED2_OFF_CODE);
  EEPROM.get(16, LED3_ON_CODE);
  EEPROM.get(20, LED3_OFF_CODE);
}

void saveIRCodes() {
  EEPROM.put(0, LED1_ON_CODE);
  EEPROM.put(4, LED1_OFF_CODE);
  EEPROM.put(8, LED2_ON_CODE);
  EEPROM.put(12, LED2_OFF_CODE);
  EEPROM.put(16, LED3_ON_CODE);
  EEPROM.put(20, LED3_OFF_CODE);
  EEPROM.commit();
}

void pushIRCode(uint32_t code) {
  for (int i = 4; i > 0; i--) {
    irCodes[i] = irCodes[i - 1];
  }
  irCodes[0] = code;
}

void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html><html><head><title>ESP LED Control</title>
    <style>body { font-family: Arial; text-align: center; margin-top: 40px; }
    button { padding: 10px 20px; margin: 10px; font-size: 16px; }
    .status { font-weight: bold; margin: 5px; }</style>
    <script>
      function toggle(led, state) {
        fetch(`/${led}/${state}`).then(() => update());
      }
      function update() {
        fetch('/status').then(res => res.json()).then(data => {
          document.getElementById('s1').innerText = data.led1;
          document.getElementById('s2').innerText = data.led2;
          document.getElementById('s3').innerText = data.led3;
        });
      }
      setInterval(update, 2000);
      window.onload = update;
    </script></head><body>
    <h1>ESP8266 LED Control</h1>
    <p><a href="/admin">Go to Admin Page</a></p>
    <p><a href="/ir-admin">Go to IR Admin Page</a></p>
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
    <!DOCTYPE html><html><head><title>ESP8266 Admin</title>
    <style>body { font-family: Arial; text-align: center; margin-top: 40px; }</style>
    <script>
      function getNetworkInfo() {
        fetch('/networkinfo')
          .then(res => res.json())
          .then(data => {
            document.getElementById('staStatus').innerText = 'STA Status: ' + data.sta_status_text;
            document.getElementById('staIP').innerText = 'STA IP: ' + data.sta_ip;
            document.getElementById('apIP').innerText = 'AP IP: ' + data.ap_ip;
          });
      }
      window.onload = getNetworkInfo;
    </script></head><body>
    <h1>ESP8266 Admin</h1>
    <p id="staStatus"></p>
    <p id="staIP"></p>
    <p id="apIP"></p>
    <p><a href="/">Back to Control Page</a></p>
    </body></html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleIRAdmin() {
  String html = R"rawliteral(
    <!DOCTYPE html><html><head><title>IR Admin</title>
    <style>body { font-family: Arial; text-align: center; margin-top: 20px; }</style>
    <script>
      function fetchIR() {
        fetch('/ir-latest').then(res => res.json()).then(data => {
          let irList = data.ir_codes.map(code => `0x${code}`).join("<br>");
          document.getElementById('latestIR').innerHTML = irList;
        });
      }
      setInterval(fetchIR, 2000);
      window.onload = fetchIR;
    </script></head><body>
    <h1>IR Admin Panel</h1>
    <p>Last 5 IR Codes Received:</p>
    <div id="latestIR">Loading...</div><br><br>
    <h2>Edit IR Hex Codes</h2>
    <form method="POST" action="/save-ir-codes">
      LED1 ON: <input name="l1on" value=")rawliteral" + String(LED1_ON_CODE, HEX) + R"rawliteral("><br>
      LED1 OFF: <input name="l1off" value=")rawliteral" + String(LED1_OFF_CODE, HEX) + R"rawliteral("><br>
      LED2 ON: <input name="l2on" value=")rawliteral" + String(LED2_ON_CODE, HEX) + R"rawliteral("><br>
      LED2 OFF: <input name="l2off" value=")rawliteral" + String(LED2_OFF_CODE, HEX) + R"rawliteral("><br>
      LED3 ON: <input name="l3on" value=")rawliteral" + String(LED3_ON_CODE, HEX) + R"rawliteral("><br>
      LED3 OFF: <input name="l3off" value=")rawliteral" + String(LED3_OFF_CODE, HEX) + R"rawliteral("><br><br>
      <input type="submit" value="Save">
    </form>
    <br><a href="/">Back to LED Control</a>
    </body></html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleLatestIR() {
  String json = "{ \"ir_codes\": [";
  for (int i = 0; i < 5; i++) {
    json += "\"" + String(irCodes[i], HEX) + "\"";
    if (i < 4) json += ",";
  }
  json += "] }";
  server.send(200, "application/json", json);
}

void handleSaveIRCodes() {
  LED1_ON_CODE  = strtoul(server.arg("l1on").c_str(), NULL, 16);
  LED1_OFF_CODE = strtoul(server.arg("l1off").c_str(), NULL, 16);
  LED2_ON_CODE  = strtoul(server.arg("l2on").c_str(), NULL, 16);
  LED2_OFF_CODE = strtoul(server.arg("l2off").c_str(), NULL, 16);
  LED3_ON_CODE  = strtoul(server.arg("l3on").c_str(), NULL, 16);
  LED3_OFF_CODE = strtoul(server.arg("l3off").c_str(), NULL, 16);
  saveIRCodes();
  server.sendHeader("Location", "/ir-admin");
  server.send(303);
}

void handleNetworkInfo() {
  String json = "{";
  json += "\"sta_status_text\":\"" + String((WiFi.status() == WL_CONNECTED) ? "Connected" : "Not Connected") + "\",";
  json += "\"sta_ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\"}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  loadIRCodes();

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid_ap, password_ap);
  dnsServer.start(53, "ctrl.me", apIP);

  WiFiManager wifiManager;
  wifiManager.autoConnect("IOTconfig-01");

  if (MDNS.begin("ctrl")) Serial.println("mDNS responder started");

  pinMode(led1, OUTPUT); digitalWrite(led1, HIGH);
  pinMode(led2, OUTPUT); digitalWrite(led2, HIGH);
  pinMode(led3, OUTPUT); digitalWrite(led3, HIGH);
  pinMode(RECV_PIN, INPUT);
  irrecv.enableIRIn();

  server.on("/", handleRoot);
  server.on("/admin", handleAdmin);
  server.on("/ir-admin", handleIRAdmin);
  server.on("/ir-latest", handleLatestIR);
  server.on("/networkinfo", handleNetworkInfo);
  server.on("/save-ir-codes", HTTP_POST, handleSaveIRCodes);
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
  dnsServer.processNextRequest();
  server.handleClient();

  if (irrecv.decode(&results)) {
    uint32_t code = results.value;
    Serial.printf("IR Received: 0x%08X\n", code);
    pushIRCode(code);

    if (code == LED1_ON_CODE) digitalWrite(led1, LOW);
    else if (code == LED1_OFF_CODE) digitalWrite(led1, HIGH);
    else if (code == LED2_ON_CODE) digitalWrite(led2, LOW);
    else if (code == LED2_OFF_CODE) digitalWrite(led2, HIGH);
    else if (code == LED3_ON_CODE) digitalWrite(led3, LOW);
    else if (code == LED3_OFF_CODE) digitalWrite(led3, HIGH);
    else Serial.println("Unknown IR code");

    irrecv.resume();
  }
}
