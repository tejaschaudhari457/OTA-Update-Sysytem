#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <esp_ota_ops.h>
#include <time.h>

// WiFi credentials
const char *ssid = "Airtel_Lithium_India";
const char *password = "Lithium@677781";

// OTA URLs
const char *versionUrl = "https://raw.githubusercontent.com/tejaschaudhari457/OTA/main/ota/version.txt";
const char *firmwareUrl = "https://raw.githubusercontent.com/tejaschaudhari457/OTA/main/ota/firmware.bin";

// Current firmware version
String currentVersion = "1.0.1";

// LED simulation
bool ledState = false;
unsigned long lastLedToggle = 0;
const unsigned long ledInterval = 1000;

// Flag to check OTA once
bool checkedAfterConnect = false;

void syncTime() {
  configTime(19800, 0, "pool.ntp.org", "time.nist.gov");
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.println("🕒 Time synced.");
  } else {
    Serial.println("⚠️ Time sync failed.");
  }
}

void performOTA() {
  WiFiClientSecure client;
  client.setTimeout(15000);
  client.setInsecure(); // ❗ Only for testing. Use real certs in production.

  Serial.println("⬇️ Starting OTA...");

  t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("❌ OTA Failed. Code: %d, Msg: %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("📭 No update found.");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("✅ OTA Success. Rebooting...");
      break;
  }
}

void checkForUpdate() {
  HTTPClient http;
  http.setTimeout(10000);
  http.begin(versionUrl);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String newVersion = http.getString();
    newVersion.trim();
    Serial.printf("🔄 Current: %s | Latest: %s\n", currentVersion.c_str(), newVersion.c_str());

    if (newVersion != currentVersion) {
      Serial.println("🆕 Update available!");
      performOTA();
    } else {
      Serial.println("✅ Firmware is up-to-date.");
    }
  } else {
    Serial.printf("❌ Version check failed. Code: %d\n", httpCode);
  }

  http.end();
}

void validateFirmware() {
  esp_ota_img_states_t otaState;
  if (esp_ota_get_state_partition(esp_ota_get_running_partition(), &otaState) == ESP_OK) {
    if (otaState == ESP_OTA_IMG_PENDING_VERIFY) {
      Serial.println("🧪 Verifying new firmware...");

      bool appValid = true; // Add checks here if needed
      if (appValid) {
        esp_ota_mark_app_valid_cancel_rollback();
        Serial.println("✅ Marked firmware as valid.");
      } else {
        Serial.println("❌ App invalid. Rolling back...");
        esp_ota_mark_app_invalid_rollback_and_reboot();
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("🔌 Booting...");

  validateFirmware();  // Check if rollback needed

  WiFi.begin(ssid, password);
  Serial.print("📶 Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\n✅ Connected!");
  Serial.println(WiFi.localIP());

  syncTime();
  checkForUpdate();
  checkedAfterConnect = true;
}

void loop() {
  // Re-check OTA on reconnection
  if (WiFi.status() == WL_CONNECTED && !checkedAfterConnect) {
    Serial.println("🔁 WiFi Reconnected. Checking OTA...");
    checkForUpdate();
    checkedAfterConnect = true;
  }

  if (WiFi.status() != WL_CONNECTED) {
    checkedAfterConnect = false;
  }

  if (millis() - lastLedToggle > ledInterval) {
    lastLedToggle = millis();
    ledState = !ledState;
    Serial.println(ledState ? "Yellow LED ON" : "Yellow LED OFF");
  }
}
