
#include <WebServer.h>
#include <WiFi.h>
#include <esp32cam.h>
#include <PubSubClient.h>

// ==== USTAWIENIA Wi-Fi ====
const char* WIFI_SSID = "LPAWiFi1";
const char* WIFI_PASS = "LPAWiFi1Password";

// ==== MQTT ====
// Jeśli mDNS (.local) nie działa, wstaw IP swojego Pi, np. "192.168.1.50"
const char* MQTT_HOST = "pi5m.local";    // lub np. "192.168.1.50"
const uint16_t MQTT_PORT = 1883;
const char* MQTT_TOPIC = "esp32cam/notify";
// const char* MQTT_USER = "twoj_user";   // jeśli używasz uwierzytelniania
// const char* MQTT_PASSW = "twoje_haslo";

WiFiClient espClient;
PubSubClient mqtt(espClient);

// ==== Serwer WWW ====
WebServer server(80);

// ==== Rozdzielczości ====
static auto loRes  = esp32cam::Resolution::find(320, 240);
static auto midRes = esp32cam::Resolution::find(350, 530);
static auto hiRes  = esp32cam::Resolution::find(800, 600);

// ==== Prototypy ====
void serveJpg();
void handleJpgLo();
void handleJpgMid();
void handleJpgHi();
void handleNotify();
void handleNotFound();
void connectMqtt();

void connectMqtt() {
  uint32_t backoff = 500; // ms
  while (!mqtt.connected() && WiFi.isConnected()) {
    String cid = "ESP32CAM-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    Serial.print("[MQTT] Łączenie jako "); Serial.println(cid);

    // Jeśli masz user/pass:
    // bool ok = mqtt.connect(cid.c_str(), MQTT_USER, MQTT_PASSW);
    bool ok = mqtt.connect(cid.c_str());
    if (ok) {
      Serial.println("[MQTT] Połączono");
      mqtt.publish(MQTT_TOPIC, "ESP32-CAM online", true); // retain
      break;
    } else {
      Serial.print("[MQTT] Próba nieudana, rc=");
      Serial.println(mqtt.state());
      delay(backoff);
      backoff = backoff < 5000 ? backoff * 2 : 5000;
    }
  }
}

void serveJpg()
{
  auto frame = esp32cam::capture();
  if (frame == nullptr) {
    Serial.println("CAPTURE FAIL");
    server.send(503, "text/plain", "CAPTURE FAIL");
    return;
  }
  // Wyłączamy spam CAPTURE OK:
  // Serial.printf("CAPTURE OK %dx%d %db\n", frame->getWidth(), frame->getHeight(),
  //               static_cast<int>(frame->size()));

  server.setContentLength(frame->size());
  server.send(200, "image/jpeg");
  WiFiClient client = server.client();
  frame->writeTo(client);
}

void handleJpgLo()
{
  if (!esp32cam::Camera.changeResolution(loRes)) {
    Serial.println("SET-LO-RES FAIL");
  }
  serveJpg();
}

void handleJpgMid()
{
  if (!esp32cam::Camera.changeResolution(midRes)) {
    Serial.println("SET-MID-RES FAIL");
  }
  serveJpg();
}

void handleJpgHi()
{
  if (!esp32cam::Camera.changeResolution(hiRes)) {
    Serial.println("SET-HI-RES FAIL");
  }
  serveJpg();
}

// ==== ENDPOINT NOTIFY ====
// Wywołanie: http://<IP>/notify?msg=detected%3A%20person(84%25)
void handleNotify()
{
  if (!server.hasArg("msg")) {
    server.send(400, "text/plain", "Missing 'msg' parameter");
    return;
  }
  String msg = server.arg("msg");
  msg.replace("\r", " ");
  msg.replace("\n", " ");

  // Czytelny log w Serial:
  Serial.print("[DETECTION] ");
  Serial.println(msg);

  // Publikacja MQTT
  if (!mqtt.connected()) {
    connectMqtt();
  }
  bool ok = false;
  if (mqtt.connected()) {
    ok = mqtt.publish(MQTT_TOPIC, msg.c_str(), true /*retain*/);
  }
  Serial.println(ok ? "[MQTT] published" : "[MQTT] publish FAILED");

  server.send(200, "text/plain", "OK");
}

// Logujemy inne żądania (diagnostyka)
void handleNotFound() {
  Serial.print("[HTTP] 404 -> ");
  Serial.println(server.uri());
  server.send(404, "text/plain", "Not Found");
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  using namespace esp32cam;
  Config cfg;
  cfg.setPins(pins::AiThinker);
  cfg.setResolution(hiRes);
  cfg.setBufferCount(2);
  cfg.setJpeg(80);

  bool ok = Camera.begin(cfg);
  Serial.println(ok ? "CAMERA OK" : "CAMERA FAIL");

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Łączenie z Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setKeepAlive(60);
  mqtt.setBufferSize(2048);
  mqtt.setSocketTimeout(10);
  connectMqtt();

  Serial.print("http://");
  Serial.println(WiFi.localIP());
  Serial.println("  /cam-lo.jpg");
  Serial.println("  /cam-mid.jpg");
  Serial.println("  /cam-hi.jpg");
  Serial.println("  /notify?msg=Hello");

  server.on("/cam-lo.jpg",  handleJpgLo);
  server.on("/cam-mid.jpg", handleJpgMid);
  server.on("/cam-hi.jpg",  handleJpgHi);
  server.on("/notify",      handleNotify);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  server.handleClient();
  mqtt.loop();

  if (!mqtt.connected() && WiFi.isConnected()) {
    connectMqtt();
  }
}
