/*[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
upload_port = COM10
monitor_port = COM10
monitor_speed = 115200
board_build.partitions = min_spiffs.csv
*/


#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include "esp_ota_ops.h"  // ✅ Needed for rollback control and partition info

// ✅ WiFi credentials
const char* ssid = "Airtel_Lithium_India";
const char* password = "Lithium@677781";

// ✅ GitHub OTA URLs (adjust as needed)
const char* versionURL  = "https://raw.githubusercontent.com/tejaschaudhari457/dummyRepo/main/ota/version.txt";
const char* firmwareURL = "https://raw.githubusercontent.com/tejaschaudhari457/dummyRepo/main/ota/firmware.bin";

// ✅ This device’s current firmware version
const String currentVersion = "1.0.0";

// ✅ Prototypes
void checkForOTAUpdate();

void setup() {
  Serial.begin(115200);
  delay(1000);

  // ✅ Show which partition we're running from (ota_0 or ota_1)
  const esp_partition_t* running = esp_ota_get_running_partition();
  Serial.printf("🔍 Currently running partition: %s\n", running->label);

  // ✅ Cancel rollback if booted successfully
  esp_ota_mark_app_valid_cancel_rollback();

  // ✅ Connect to WiFi
  Serial.printf("Connecting to WiFi: %s", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected ✅");

  // ✅ Check for OTA update
  checkForOTAUpdate();
}

void loop() {
  Serial.println("✅ Blue LED ON");
  delay(500);
  Serial.println("Blue LED OFF");
  delay(1500);
}

// ✅ OTA update logic
void checkForOTAUpdate() {
  Serial.println("\n📡 Checking for OTA update...");

  HTTPClient http;
  http.begin(versionURL);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String latestVersion = http.getString();
    latestVersion.trim();

    Serial.printf("🔢 Latest version: %s\n", latestVersion.c_str());
    Serial.printf("📦 Current version: %s\n", currentVersion.c_str());

    if (latestVersion != currentVersion) {
      Serial.println("🚀 New version found! Starting OTA...");

      http.end();
      http.begin(firmwareURL);
      int firmwareCode = http.GET();

      if (firmwareCode == 200) {
        int contentLength = http.getSize();
        WiFiClient& client = http.getStream();

        if (Update.begin(contentLength)) {
          uint8_t buff[128] = { 0 };
          int written = 0;
          unsigned long lastPrint = 0;

          while (http.connected() && written < contentLength) {
            size_t available = client.available();
            if (available) {
              int readLen = client.readBytes(buff, (available > sizeof(buff)) ? sizeof(buff) : available);
              Update.write(buff, readLen);
              written += readLen;

              // ✅ Print progress every ~500ms
              unsigned long now = millis();
              if (now - lastPrint > 500 || written == contentLength) {
                int percent = (written * 100) / contentLength;
                Serial.printf("📦 Update Progress: %d%%\r\n", percent);
                lastPrint = now;
              }
            }
            delay(1);
          }

          if (Update.end()) {
            Serial.println("\n✅ OTA Update complete");

            if (Update.isFinished()) {
              Serial.println("🔁 Rebooting to new firmware...");
              delay(2000);
              ESP.restart();
            } else {
              Serial.println("❌ OTA Update incomplete.");
            }
          } else {
            Serial.printf("❌ Update failed. Error #%d\n", Update.getError());
          }

        } else {
          Serial.println("❌ Not enough space for OTA");
        }

      } else {
        Serial.printf("❌ Firmware URL failed. HTTP code: %d\n", firmwareCode);
      }

    } else {
      Serial.println("✅ Already up to date.");
    }

  } else {
    Serial.printf("❌ Version check failed. HTTP code: %d\n", httpCode);
  }

  http.end();
}
