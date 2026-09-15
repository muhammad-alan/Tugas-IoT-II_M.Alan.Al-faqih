const byte ldrPin = A0; 

void setup() {  
  Serial.begin(115200);  
}

void loop() {  
  int ldrValue = analogRead(ldrPin);   
    
  Serial.print("Intensitas Cahaya (ADC): ");  
  Serial.println(ldrValue);  
    
  delay(1000);   
}