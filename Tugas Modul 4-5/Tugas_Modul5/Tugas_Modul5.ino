#include <painlessMesh.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define MESH_PREFIX     "LabIoTMesh"
#define MESH_PASSWORD   "iotmeshpassword"
#define MESH_PORT       5555

#define DHTPIN          2       // Pin D4 (GPIO 2)
#define DHTTYPE         DHT22   // Ubah ke DHT22 jika menggunakan varian DHT22

DHT dht(DHTPIN, DHTTYPE);
Scheduler userScheduler;
painlessMesh mesh;

void sendMessage();
Task taskSendMessage(TASK_SECOND * 3, TASK_FOREVER, &sendMessage);

void sendMessage() {
  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();

  if (isnan(suhu) ||isnan(kelembapan)) {
    Serial.println("[DHT Error] Gagal membaca data dari sensor DHT!");
    return;
  }

  // Kemas data ke format JSON sesuai spesifikasi tugas
  StaticJsonDocument<200> doc;
  doc["tipe"] = "suhu_node";
  doc["suhu"] = suhu;
  doc["kelembapan"] = kelembapan;

  String msg;
  serializeJson(doc, msg);
  mesh.sendBroadcast(msg);

  Serial.print("[KIRIM DHT] ");
  Serial.println(msg);
}

void receivedCallback(uint32_t from, String &msg) {
  // Node 1 dapat membiarkan callback ini kosong atau mencetak pesan masuk jika diperlukan
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("--> Koneksi Baru Terdeteksi! Node ID: %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.println("--> Topologi mesh diperbarui");
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);

  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

  Serial.println("Node 1 (Sensor DHT) Siap!");
}

void loop() {
  mesh.update();
}