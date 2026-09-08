const int buttonPin = 4;       // LED terhubung ke GPIO 4
const int ledPin = 5;    // Push button terhubung ke GPIO 5

bool ledState = false;

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);

  // Kondisi awal LED mati
  digitalWrite(ledPin, LOW);

  Serial.println("=== SISTEM SAKELAR TOGGLE ESP32 ===");
  Serial.println("LED awal: OFF");
}

void loop() {
  int buttonState = digitalRead(buttonPin);

  // Jika tombol ditekan
  if (buttonState == HIGH) {

    // Mengubah kondisi LED
    ledState = !ledState;

    digitalWrite(ledPin, ledState);

    if (ledState) {
      Serial.println("Tombol ditekan -> LED ON");
    } else {
      Serial.println("Tombol ditekan -> LED OFF");
    }

    // Debounce untuk mencegah satu tekanan
    // terbaca berkali-kali
    delay(200);

    // Menunggu tombol dilepas
    while (digitalRead(buttonPin) == HIGH) {
      delay(10);
    }
  }
}