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
  <h1>ESP8266 Web Server</h1>  
  <div class="sensor-box">  
    <p>Suhu Saat Ini: <strong>%TEMPERATURE%</strong> Celcius</p>  
  </div>  
  <h2>Kendali Relay</h2>  
  <a href="/relay/on"><button class="btn-on">ON</button></a>  
  <a href="/relay/off"><button class="btn-off">OFF</button></a>  
</body>  
</html>  
)rawliteral";

void handleRoot() {  
  String html = index_html; // Salin kerangka HTML ke variabel dinamis
  float t = dht.readTemperature();  
  
  // Mengganti teks placeholder %TEMPERATURE% dengan suhu nyata
  if (isnan(t)) {
    html.replace("%TEMPERATURE%", "--"); 
  } else {
    html.replace("%TEMPERATURE%", String(t)); 
  }
  
  server.send(200, "text/html", html);  
}

void handleRelayOn() {  
  digitalWrite(relayPin, HIGH); 
  server.sendHeader("Location", "/");   
  server.send(303);  
}

void handleRelayOff() {  
  digitalWrite(relayPin, LOW);   
  server.sendHeader("Location", "/");   
  server.send(303);  
}

void setup() {  
  Serial.begin(115200);  
  pinMode(relayPin, OUTPUT);  
  digitalWrite(relayPin, LOW);  
  dht.begin();  
    
  WiFi.mode(WIFI_STA);   
  WiFi.begin(ssid, password);  
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }  
  Serial.println("\nIP Address Server Anda: ");  
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);  
  server.on("/relay/on", handleRelayOn);  
  server.on("/relay/off", handleRelayOff);  
  server.begin();  
}

void loop() {  
  server.handleClient();  
}