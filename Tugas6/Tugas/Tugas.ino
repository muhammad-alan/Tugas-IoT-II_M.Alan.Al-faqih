#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// Wi-Fi
const char* ssid = "***";
const char* password = "12345678";

// MQTT broker
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char* clientId = "ESP8266_LabIoT_25782074";

// Topik MQTT
const char* topicTelemetry = "kampus/labiot/25782074/telemetri";
const char* topicControl   = "kampus/labiot/25782074/kendali";
const char* topicAlert     = "kampus/labiot/25782074/alert";
const char* topicStatus    = "kampus/labiot/25782074/status";

// Pin perangkat
const byte dhtPin = 2;  // D4 / GPIO2
const byte ledPin = 5;  // D1 / GPIO5
#define DHTTYPE DHT22

DHT dht(dhtPin, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

// Timer
unsigned long lastPublish = 0;
unsigned long lastWiFiAttempt = 0;
const unsigned long interval = 4000;

// Status keselamatan
bool alertSent = false;
bool previousDanger = false;

void publishStatus(const char* status, const char* info) {
  StaticJsonDocument<192> doc;
  doc["status"] = status;
  doc["keterangan"] = info;

  char buffer[192];
  serializeJson(doc, buffer);
  client.publish(topicStatus, buffer, true);
}

// Callback kendali LED manual
void callback(char* topic, byte* payload, unsigned int length) {
  char message[128];

  if (length >= sizeof(message)) return;

  memcpy(message, payload, length);
  message[length] = '\0';

  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, message)) return;

  if (!doc["led"].is<int>()) return;

  int value = doc["led"];

  // Kendali manual hanya saat suhu aman
  if (!previousDanger && (value == 0 || value == 1)) {
    digitalWrite(ledPin, value ? HIGH : LOW);
    Serial.println(value ? "LED MENYALA (MANUAL)" :
                           "LED PADAM (MANUAL)");
  } else if (previousDanger) {
    Serial.println("Kendali manual ditolak: suhu berbahaya!");
  }
}

// Koneksi dan pendaftaran LWT
void reconnectMQTT() {
  while (WiFi.status() == WL_CONNECTED &&
         !client.connected()) {

    Serial.println("Menghubungkan MQTT...");

    StaticJsonDocument<128> willDoc;
    willDoc["status"] = "OFFLINE";
    willDoc["keterangan"] =
        "Perangkat Terputus Tak Terduga";

    char willMessage[128];
    serializeJson(willDoc, willMessage);

    if (client.connect(
          clientId,
          topicStatus,
          1,
          true,
          willMessage)) {

      Serial.println("MQTT terhubung!");

      client.subscribe(topicControl);
      publishStatus("ONLINE",
                    "Perangkat Beroperasi Normal");

      Serial.println("Topik kendali aktif.");
    } else {
      Serial.print("Koneksi gagal, kode: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Menghubungkan Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi terhubung!");
  Serial.println(WiFi.localIP());

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  client.setBufferSize(512);
}

void loop() {
  // Memulihkan koneksi Wi-Fi jika terputus
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWiFiAttempt >= 5000) {
      lastWiFiAttempt = millis();
      WiFi.reconnect();
    }
    return;
  }

  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();

  // Kirim telemetri setiap 4 detik
  if (millis() - lastPublish < interval) return;
  lastPublish = millis();

  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();

  if (isnan(suhu) || isnan(kelembapan)) {
    Serial.println("Gagal membaca DHT22!");
    return;
  }

  bool danger = suhu > 31.0;

  // Logika keselamatan otomatis
  if (danger) {
    digitalWrite(ledPin, HIGH);

    // Kirim peringatan saat pertama memasuki kondisi bahaya
    if (!alertSent) {
      StaticJsonDocument<192> alert;
      alert["status"] = "BAHAYA";
      alert["parameter"] = "OVERHEAT";
      alert["nilai_suhu"] = suhu;

      char alertBuffer[192];
      serializeJson(alert, alertBuffer);
      client.publish(topicAlert, alertBuffer);

      Serial.println("PERINGATAN SUHU TINGGI!");
      alertSent = true;
    }
  } else {
    // Reset peringatan setelah suhu kembali aman
    alertSent = false;

    // Padamkan LED jika sebelumnya menyala karena bahaya
    if (previousDanger) {
      digitalWrite(ledPin, LOW);
    }
  }

  previousDanger = danger;

  // Susun JSON telemetri
  StaticJsonDocument<256> telemetry;
  telemetry["node_id"] = clientId;
  telemetry["suhu"] = suhu;
  telemetry["kelembapan"] = kelembapan;
  telemetry["status_led"] = digitalRead(ledPin);
  telemetry["kondisi_suhu"] = danger ? "PANAS" : "NORMAL";

  char buffer[256];
  serializeJson(telemetry, buffer);

  if (client.publish(topicTelemetry, buffer)) {
    Serial.print("Telemetri: ");
    Serial.println(buffer);
  } else {
    Serial.println("Gagal mengirim telemetri!");
  }
}