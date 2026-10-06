#include <painlessMesh.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define MESH_PREFIX   "Lelaki_Bingung"
#define MESH_PASSWORD "Cihuy"
#define MESH_PORT     7777

// Konfigurasi pin dan tipe sensor DHT
#define DHTPIN 2       // Pin D4 (GPIO 2)
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Identitas node
const char* nodeName = "Alan_Bingung";

Scheduler userScheduler;
painlessMesh mesh;

// Prototipe fungsi
void sendMessage();
void receivedCallback(uint32_t from, String &msg);

Task taskSendMessage(
  TASK_SECOND * 3,
  TASK_FOREVER,
  &sendMessage
);

void sendMessage() {
  // Membaca data suhu dan kelembapan dari sensor DHT
  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();

  // Validasi pembacaan sensor
  if (isnan(suhu) || isnan(kelembapan)) {
    Serial.println(
      "[DHT Error] Gagal membaca data dari sensor DHT!"
    );
    return;
  }

  // Membuat data telemetri dalam format JSON
  StaticJsonDocument<200> doc;

  doc["node"] = nodeName;
  doc["chipId"] = mesh.getNodeId();
  doc["suhu"] = suhu;
  doc["kelembapan"] = kelembapan;

  String msg;
  serializeJson(doc, msg);

  // Mengirim data ke jaringan mesh
  mesh.sendBroadcast(msg);

  Serial.print("[KIRIM MESH] ");
  Serial.println(msg);

  // Memproses data sendiri untuk pengujian filter
  receivedCallback(mesh.getNodeId(), msg);
}

// Callback untuk menerima dan memfilter telemetri
void receivedCallback(uint32_t from, String &msg) {
  // Parsing data JSON
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, msg);

  if (!error) {
    const char* sender = doc["node"];
    uint32_t chipId = doc["chipId"];
    float suhu = doc["suhu"];
    float kelembapan = doc["kelembapan"];

    // Jika suhu lebih dari 32°C
    if (suhu > 32.0) {
      Serial.println("========================================");

      Serial.printf(
        "[TERIMA DARI] %s (Node ID: %u | Chip ID: %u)\n",
        sender,
        from,
        chipId
      );

      Serial.printf(
        "Suhu       : %.2f °C [KRITIS / TINGGI]\n",
        suhu
      );

      Serial.printf(
        "Kelembapan : %.2f %%\n",
        kelembapan
      );

      Serial.println("========================================");
    } else {
      // Jika suhu normal atau sama dengan 32°C
      Serial.printf(
        "[NORMAL] Telemetri dari Node %u aman.\n",
        chipId
      );
    }
  } else {
    // Jika data yang diterima bukan JSON valid
    Serial.printf(
      "[TERIMA DATA MENTAH DARI %u]: %s\n",
      from,
      msg.c_str()
    );
  }
}

// Callback saat node baru bergabung
void newConnectionCallback(uint32_t nodeId) {
  Serial.printf(
    "--> Koneksi Baru Terdeteksi! Node ID: %u\n",
    nodeId
  );
}

// Callback saat topologi mesh berubah
void changedConnectionCallback() {
  Serial.println(
    "--> Topologi rantai mesh telah diperbarui"
  );
}

void setup() {
  Serial.begin(115200);

  // Inisialisasi sensor DHT
  dht.begin();

  // Mengatur level log debugging
  mesh.setDebugMsgTypes(ERROR | STARTUP);

  // Inisialisasi jaringan mesh
  mesh.init(
    MESH_PREFIX,
    MESH_PASSWORD,
    &userScheduler,
    MESH_PORT
  );

  // Mendaftarkan callback
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);

  // Menambahkan task pengiriman data
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

  Serial.printf(
    "Mesh Node [%s] Berjalan. Menunggu pembentukan topologi...\n",
    nodeName
  );
}

void loop() {
  // Memproses jaringan mesh
  mesh.update();
}