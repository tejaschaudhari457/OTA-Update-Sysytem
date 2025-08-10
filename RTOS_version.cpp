/*[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

build_flags = -D FW_VERSION="1.0.0"
board_build.partitions = default_ota.csv

; Serial upload/monitor settings (change COM10 jaruri asel)
upload_port = COM10
upload_speed = 921600

monitor_port = COM10
monitor_speed = 115200
monitor_filters = direct, esp32_exception_decoder
*/  


#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include "esp_ota_ops.h"

// -------------------- Build-time version injection --------------------
#ifndef FW_VERSION
#define FW_VERSION "0.0.0"
#endif
const String currentVersion = FW_VERSION;

// -------------------- WiFi credentials --------------------
const char* ssid = "Airtel_Lithium_India";
const char* password = "Lithium@677781";

// -------------------- OTA URLs --------------------
const char* versionURL  = "https://raw.githubusercontent.com/tejaschaudhari457/dummyRepo/main/ota/version.txt";
const char* firmwareURL = "https://raw.githubusercontent.com/tejaschaudhari457/dummyRepo/main/ota/firmware.bin";

// -------------------- LED Pin --------------------
#define LED_PIN 2

// -------------------- FreeRTOS Task Handles --------------------
TaskHandle_t otaTaskHandle;
TaskHandle_t ledTaskHandle;

// -------------------- Function Prototypes --------------------
void checkForOTAUpdate();
bool isNewerVersion(const String &latest, const String &current);
bool downloadFirmware();
void ledTask(void* parameter);
void otaTask(void* parameter);

// ====================================================================
// SETUP
// ====================================================================
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  const esp_partition_t* running = esp_ota_get_running_partition();
  Serial.printf("🔍 Currently running partition: %s\n", running->label);

  // Mark firmware as valid if boot successful
  esp_ota_mark_app_valid_cancel_rollback();

  // Connect to WiFi
  Serial.printf("Connecting to WiFi: %s", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" ✅ connected");

  // Create LED blink task on Core 1
  xTaskCreatePinnedToCore(ledTask, "LED Blink Task", 2048, NULL, 1, &ledTaskHandle, 1);

  // Create OTA task on Core 0
  xTaskCreatePinnedToCore(otaTask, "OTA Task", 8192, NULL, 1, &otaTaskHandle, 0);
}

void loop() {
  // Nothing here — all work is in tasks
}

// ====================================================================
// LED Blink Task
// ====================================================================
void ledTask(void* parameter) {
  while (1) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("✅ LED ON");
    vTaskDelay(500 / portTICK_PERIOD_MS);
    digitalWrite(LED_PIN, LOW);
    Serial.println("LED OFF");
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// ====================================================================
// OTA Task
// ====================================================================
void otaTask(void* parameter) {
  checkForOTAUpdate();
  vTaskDelete(NULL); // Delete OTA task after completion
}

// ====================================================================
// Version Comparison
// ====================================================================
bool isNewerVersion(const String &latest, const String &current) {
  int lmj, lmn, lpt;
  int cmj, cmn, cpt;
  sscanf(latest.c_str(), "%d.%d.%d", &lmj, &lmn, &lpt);
  sscanf(current.c_str(), "%d.%d.%d", &cmj, &cmn, &cpt);

  if (lmj > cmj) return true;
  if (lmj < cmj) return false;
  if (lmn > cmn) return true;
  if (lmn < cmn) return false;
  if (lpt > cpt) return true;
  return false;
}

// ====================================================================
// OTA Update Logic
// ====================================================================
void checkForOTAUpdate() {
  for (int attempt = 1; attempt <= 3; attempt++) {
    Serial.printf("\n📡 Checking for OTA update (Attempt %d)...\n", attempt);

    HTTPClient http;
    http.begin(versionURL);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String latestVersion = http.getString();
      latestVersion.trim();

      Serial.printf("🔢 Latest version: %s\n", latestVersion.c_str());
      Serial.printf("📦 Current version: %s\n", currentVersion.c_str());

      if (isNewerVersion(latestVersion, currentVersion)) {
        Serial.println("🚀 Newer version detected! Starting OTA...");
        http.end();

        if (downloadFirmware()) {
          Serial.println("🔁 Rebooting to new firmware...");
          delay(2000);
          ESP.restart();
        } else {
          Serial.println("❌ OTA update failed. Rolling back...");
          esp_ota_mark_app_invalid_rollback_and_reboot();
        }
      } else {
        Serial.println("✅ Already up to date.");
      }
      http.end();
      return;
    } 
    else {
      Serial.printf("❌ Version check failed. HTTP code: %d\n", httpCode);
    }

    http.end();
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}

// ====================================================================
// Firmware Download
// ====================================================================
bool downloadFirmware() {
  HTTPClient http;
  http.begin(firmwareURL);
  int firmwareCode = http.GET();

  if (firmwareCode == 200) {
    int contentLength = http.getSize();
    if (contentLength <= 0) {
      Serial.println("❌ Invalid Content-Length from server");
      return false;
    }

    WiFiClient& client = http.getStream();

    if (Update.begin(contentLength)) {
      uint8_t buff[512] = { 0 };
      int written = 0;
      unsigned long lastPrint = 0;

      while (http.connected() && written < contentLength) {
        size_t available = client.available();
        if (available) {
          int readLen = client.readBytes(buff, (available > sizeof(buff)) ? sizeof(buff) : available);
          Update.write(buff, readLen);
          written += readLen;

          unsigned long now = millis();
          if (now - lastPrint > 500 || written == contentLength) {
            int percent = (written * 100) / contentLength;
            Serial.printf("📦 Update Progress: %d%%\n", percent);
            lastPrint = now;
          }
        }
        vTaskDelay(1); // Allow LED task to run
      }

      if (Update.end()) {
        if (Update.isFinished()) {
          Serial.println("✅ OTA Update complete");
          return true;
        } else {
          Serial.println("❌ OTA Update incomplete.");
          return false;
        }
      } else {
        Serial.printf("❌ Update failed. Error #%d\n", Update.getError());
        return false;
      }

    } else {
      Serial.println("❌ Not enough space for OTA");
      return false;
    }

  } else {
    Serial.printf("❌ Firmware URL failed. HTTP code: %d\n", firmwareCode);
    return false;
  }

  http.end();
  return false;
}
