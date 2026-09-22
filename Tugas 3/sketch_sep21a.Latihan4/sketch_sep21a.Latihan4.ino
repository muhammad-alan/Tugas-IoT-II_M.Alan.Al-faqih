#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <DHT.h>

const char* ssid = "oiiu";
const char* password = "22222229";

const char* serverName = "http://10.31.187.112/relay/on";

const byte dhtPin = 2;   // D4 / GPIO2
DHT dht(dhtPin, DHT22);

void setup() {
  Serial.begin(115200);

  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Menghubungkan ke Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Client Terhubung ke Wi-Fi!");
  Serial.print("IP Client: ");
  Serial.println(WiFi.localIP());
}

void loop() {

  // Membaca suhu dari DHT22
  float temperature = dht.readTemperature();

  // Mengecek apakah sensor berhasil dibaca
  if (isnan(temperature)) {
    Serial.println("Gagal membaca suhu DHT22!");
    delay(2000);
    return;
  }

  Serial.print("Suhu: ");
  Serial.print(temperature);
  Serial.println(" C");

  // Jika suhu lebih dari 35 C, kirim HTTP GET
  if (WiFi.status() == WL_CONNECTED && temperature > 30) {

    WiFiClient client;
    HTTPClient http;

    Serial.println("Suhu > 30 C");
    Serial.println("Mengirim HTTP GET...");

    http.begin(client, serverName);

    int httpResponseCode = http.GET();

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    http.end();

    delay(10000);
  }
  else {
    Serial.println("Suhu <= 30 C, tidak mengirim GET.");
  }

  delay(2000);
}