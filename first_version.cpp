#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include "esp_ota_ops.h"

// ✅ WiFi credentials
const char* ssid = "Airtel_Lithium_India";
const char* password = "Lithium@677781";

// ✅ GitHub OTA URLs
const char* versionURL  = "https://raw.githubusercontent.com/tejaschaudhari457/dummyRepo/main/ota/version.txt";
const char* firmwareURL = "https://raw.githubusercontent.com/tejaschaudhari457/dummyRepo/main/ota/firmware.bin";

// ✅ This device’s current firmware version
const String currentVersion = "1.0.0";

// ========== Function Prototypes ==========
void checkForOTAUpdate();
bool isNewerVersion(const String &latest, const String &current);
bool downloadFirmware();

// =========================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Show current partition
  const esp_partition_t* running = esp_ota_get_running_partition();
  Serial.printf("🔍 Currently running partition: %s\n", running->label);

  // Cancel rollback if boot is successful
  esp_ota_mark_app_valid_cancel_rollback();

  // Connect to WiFi
  Serial.printf("Connecting to WiFi: %s", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" ✅ connected");

  // OTA update check
  checkForOTAUpdate();
}

void loop() {
  Serial.println("✅ green LED ON");
  delay(500);
  Serial.println("green LED OFF");
  delay(1500);
}

// =========================================================
// Compare versions numerically (major.minor.patch)
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

// =========================================================
// OTA Update Logic
void checkForOTAUpdate() {
  for (int attempt = 1; attempt <= 3; attempt++) { // retry logic
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
          Serial.println("❌ OTA update failed.");
        }
      } else {
        Serial.println("✅ Already up to date.");
      }
      http.end();
      return; // stop retry if version check worked
    } 
    else {
      Serial.printf("❌ Version check failed. HTTP code: %d\n", httpCode);
    }

    http.end();
    delay(3000); // wait before retry
  }
}

// =========================================================
// Download and apply firmware
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

    // Start OTA update
    if (Update.begin(contentLength)) {
      uint8_t buff[1024] = { 0 }; // bigger buffer for speed
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
        delay(1);
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
