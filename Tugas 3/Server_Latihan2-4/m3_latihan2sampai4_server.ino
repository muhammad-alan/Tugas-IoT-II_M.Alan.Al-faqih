#include <ESP8266WiFi.h>  
#include <ESP8266WebServer.h>  
#include <DHT.h>

const char* ssid = "oiiu";  
const char* password = "22222229";

ESP8266WebServer server(80);

const byte dhtPin = 2;         
const byte relayPin = 12;      
DHT dht(dhtPin, DHT22);

const char index_html[] PROGMEM = R"rawliteral(  
<!DOCTYPE html>  
<html>  
<head>  
  <meta name="viewport" content="width=device-width, initial-scale=1">  
  <title>IoT Dashboard</title>  
  <style>  
    body { font-family: Arial; text-align: center; margin-top: 50px; }  
    button { padding: 15px 30px; font-size: 20px; border-radius: 8px; margin: 10px; cursor: pointer;}  
    .btn-on { background-color: #4CAF50; color: white; border: none; }  
    .btn-off { background-color: #f44336; color: white; border: none; }  
    .sensor-box { font-size: 24px; font-weight: bold; }  
  </style>  
</head>  
<body>  
  <h1>Web Server RIKI</h1>  
  <div class="sensor-box">  
    <p>Suhu Saat Ini: <strong>%TEMPERATURE%</strong> Celcius</p>
    <p>Kelembapan: <strong>%HUMIDITY%</strong> %</p>
  </div>  
  <h2>Kendali Relay</h2>  
  %RELAY_BUTTON%
</body>  
</html>  
)rawliteral";

void handleRoot() {
  String html = index_html;

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  // Menampilkan suhu
  if (isnan(t)) {
    html.replace("%TEMPERATURE%", "--");
  } else {
    html.replace("%TEMPERATURE%", String(t));
  }

  // Menampilkan kelembapan
  if (isnan(h)) {
    html.replace("%HUMIDITY%", "--");
  } else {
    html.replace("%HUMIDITY%", String(h));
  }

  // Membuat tombol dinamis berdasarkan kondisi relay/LED
  if (digitalRead(relayPin) == LOW) {
    html.replace(
      "%RELAY_BUTTON%",
      "<a href=\"/relay/on\"><button class=\"btn-on\">NYALAKAN (ON)</button></a>"
    );
  } else {
    html.replace(
      "%RELAY_BUTTON%",
      "<a href=\"/relay/off\"><button class=\"btn-off\">MATIKAN (OFF)</button></a>"
    );
  }

  server.send(200, "text/html", html);
}