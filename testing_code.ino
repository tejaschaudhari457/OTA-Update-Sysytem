#include <Arduino.h>

void setup() {
  Serial.begin(115200); // Start Serial Monitor
}

void loop() {
  Serial.println("Yellow LED ON");
  delay(500);
  Serial.println("Yellow LED OFF");
  delay(1500);
}


/*
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
upload_port = COM10
monitor_port = COM10
monitor_speed = 115200
build_flags = 
  -DCORE_DEBUG_LEVEL=5*/
