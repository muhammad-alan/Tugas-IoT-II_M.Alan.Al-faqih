#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>

const char* ssid = "***";
const char* password = "12345678";

// Konfigurasi Pin
const byte dhtPin = 2;        // D4 (GPIO 2)
const byte buttonPin = 4;     // D2 (GPIO 4)
const byte ledPin = 12;       // D6 (GPIO 12) - LED Indikator On/Off awal
const byte pwmLedPin = 5;     // D1 (GPIO 5) - LED Dimmer (PWM Slider)

DHT dht(dhtPin, DHT22);

// Variabel Pelacak Status (State & Cache)
bool ledState = false;
int pwmValue = 0;             // Variabel penyimpan nilai PWM slider (0 - 1023)
String currentTemp = "--";
String currentHum = "--";     
int buttonState;
int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
unsigned long lastTime = 0;

// Inisialisasi Async Web Server (port 80) & WebSocket (rute /ws)
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ---------------- HTML & JAVASCRIPT (FRONT-END) ----------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Smart Room Riki - PWM Dimmer</title>
  <style>
    body { font-family: Arial; text-align: center; }
    .card { background: #f0f0f0; margin: 20px auto; padding: 20px; max-width: 300px; border-radius: 10px; }
    button { padding: 15px 30px; font-size: 20px; border-radius: 5px; cursor: pointer; color: white;}  
    .btn-on { background-color: #4CAF50; }  
    .btn-off { background-color: #f44336; }  
    input[type=range] { width: 100%; cursor: pointer; }
  </style>
</head>
<body>
  <h1>Smart Room Alan</h1>
  
  <div class="card">
    <h2>Suhu: <span id="tempValue">--</span> &deg;C</h2>
  </div>

  <div class="card">
    <h2>Kelembapan: <span id="humValue">--</span> %</h2>
  </div>

  <div class="card">
    <h2>LED Indikator: <span id="ledStatus">OFF</span></h2>
    <button id="toggleBtn" class="btn-off" onclick="toggleLed()">Turn ON</button>
  </div>

  <!-- Kartu Kontrol Slider PWM (Tugas Sistem Kendali Intensitas Cahaya) -->
  <div class="card">
    <h2>LED Dimmer</h2>
    <p>Intensitas: <span id="pwmValueText">0</span></p>
    <input type="range" min="0" max="1023" id="pwmSlider" value="0" oninput="updateSliderText(this.value)" onchange="sendPWM(this.value)">
  </div>

  <script>
    var gateway = `ws://${window.location.hostname}/ws`;
    var websocket;

    window.addEventListener('load', onLoad);
    function onLoad(event) { initWebSocket(); }

    function initWebSocket() {
      websocket = new WebSocket(gateway);
      websocket.onopen    = onOpen;
      websocket.onclose   = onClose;
      websocket.onmessage = onMessage;
    }

    function onOpen(event) { console.log('WebSocket Terkoneksi'); }
    function onClose(event) { setTimeout(initWebSocket, 2000); }

    function toggleLed(){  
      websocket.send('toggle');
    }

    // Fungsi interaktif memperbarui teks angka slider secara langsung di layar
    function updateSliderText(val) {
      document.getElementById('pwmValueText').innerHTML = val;
    }

    // Fungsi mengirimkan perintah PWM ke NodeMCU via WebSocket
    function sendPWM(val) {
      websocket.send('pwm,' + val);
    }

    function onMessage(event) {
      var dataObj = JSON.parse(event.data);  
        
      if(dataObj.suhu !== undefined) {  
         document.getElementById('tempValue').innerHTML = dataObj.suhu;  
      }  

      if(dataObj.hum !== undefined) {  
         document.getElementById('humValue').innerHTML = dataObj.hum;  
      }  
        
      if(dataObj.led !== undefined) {  
         var btn = document.getElementById('toggleBtn');  
         var status = document.getElementById('ledStatus');  
         if(dataObj.led == "1"){  
           status.innerHTML = "ON";  
           btn.innerHTML = "Turn OFF";  
           btn.className = "btn-on";  
         } else {  
           status.innerHTML = "OFF";  
           btn.innerHTML = "Turn ON";  
           btn.className = "btn-off";  
         }  
      }

      // Sinkronisasi posisi slider jika ada pembaruan status dari server
      if(dataObj.pwm !== undefined) {
         document.getElementById('pwmSlider').value = dataObj.pwm;
         document.getElementById('pwmValueText').innerHTML = dataObj.pwm;
      }
    }  
  </script>  
</body>  
</html>  
)rawliteral";

// ---------------- BACK-END & WEBSOCKET LOGIC ----------------

void notifyClients() {
  String jsonString = "{\"led\":\"" + String(ledState ? 1 : 0) + "\", ";
  jsonString += "\"suhu\":\"" + currentTemp + "\", ";
  jsonString += "\"hum\":\"" + currentHum + "\", ";
  jsonString += "\"pwm\":\"" + String(pwmValue) + "\"}";
  ws.textAll(jsonString);
}

// Handler pesan masuk, termasuk parsing string data "pwm,nilai" dari slider
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message = (char*)data;
    
    if (message == "toggle") {
      ledState = !ledState;
      notifyClients();
    }
    // Deteksi awalan string "pwm," yang dikirimkan oleh slider web
    else if (message.startsWith("pwm,")) {
      int commaIndex = message.indexOf(',');
      if (commaIndex != -1) {
        String pwmString = message.substring(commaIndex + 1);
        pwmValue = pwmString.toInt(); // Konversi substring teks ke nilai integer murni
        analogWrite(pwmLedPin, pwmValue); // Terapkan intensitas cahaya ke pin D1 (GPIO 5)
        notifyClients(); // Sebarkan status pembaruan ke seluruh klien terhubung
      }
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("Client WebSocket #%u terhubung\n", client->id());
      notifyClients(); 
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("Client WebSocket #%u terputus\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(pwmLedPin, OUTPUT); // Konfigurasi pin D1 sebagai output PWM
  
  digitalWrite(ledPin, LOW);
  analogWrite(pwmLedPin, 0);  // Inisialisasi awal LED Dimmer mati (0)
  
  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nIP Address: " + WiFi.localIP().toString());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.begin();
}

void loop() {
  ws.cleanupClients();

  // 1. Eksekusi perangkat keras LED Indikator On/Off
  digitalWrite(ledPin, ledState ? HIGH : LOW);

  // 2. Baca Tombol Fisik (Debounce)
  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == HIGH) {
        ledState = !ledState;
        notifyClients(); 
      }
    }
  }
  lastButtonState = reading;

  // 3. Baca DHT (Suhu & Kelembapan) setiap 3 detik secara non-blocking
  if ((millis() - lastTime) > 3000) {
    float t = dht.readTemperature();
    float h = dht.readHumidity(); 
    
    bool dataUpdated = false;
    if(!isnan(t)) {
      currentTemp = String(t);
      dataUpdated = true;
    }
    if(!isnan(h)) {
      currentHum = String(h);   
      dataUpdated = true;
    }
    
    if(dataUpdated) {
      notifyClients();          
    }
    lastTime = millis();
  }
}