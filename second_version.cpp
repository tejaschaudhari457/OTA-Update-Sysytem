#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <esp_task_wdt.h>
#include <time.h>

// -------------------- WiFi Configuration --------------------
const char *ssid = "Airtel_Lithium_India";
const char *password = "Lithium@677781";

// -------------------- OTA URLs --------------------
const char *versionUrl = "https://raw.githubusercontent.com/tejaschaudhari457/OTA/main/ota/version.txt";
const char *firmwareUrl = "https://raw.githubusercontent.com/tejaschaudhari457/OTA/main/ota/firmware.bin";

// -------------------- Firmware Info --------------------
String currentVersion = "1.0.1";

// -------------------- LED Simulation --------------------
bool ledState = false;
unsigned long lastLedToggle = 0;
const unsigned long ledInterval = 1000;

// -------------------- Flags --------------------
bool checkedAfterConnect = false;

// -------------------- Declarations --------------------
void syncTime();
void checkForUpdate();
void performOTA();
void validateFirmware();

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  Serial.println("🔌 Booting...");

  WiFi.begin(ssid, password);
  Serial.print("📶 Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ Connected to WiFi!");
  Serial.println(WiFi.localIP());

  // DNS Check
  IPAddress githubIP;
  if (WiFi.hostByName("raw.githubusercontent.com", githubIP)) {
    Serial.print("🔗 DNS OK: ");
    Serial.println(githubIP);
  } else {
    Serial.println("❌ DNS Lookup Failed!");
  }

  // Sync NTP time
  syncTime();

  // Init watchdog (5s timeout)
  esp_task_wdt_init(5, true);
  esp_task_wdt_add(NULL);

  // Check for OTA once after WiFi connect
  checkForUpdate();
  checkedAfterConnect = true;
}

// -------------------- Loop --------------------
void loop() {
  esp_task_wdt_reset();  // Feed the watchdog

  // Recheck OTA if reconnected
  if (WiFi.status() == WL_CONNECTED && !checkedAfterConnect) {
    Serial.println("🔁 WiFi Reconnected. Checking OTA...");
    checkForUpdate();
    checkedAfterConnect = true;
  }

  // Reset flag on disconnect
  if (WiFi.status() != WL_CONNECTED) {
    checkedAfterConnect = false;
  }

  // Simulate LED
  if (millis() - lastLedToggle >= ledInterval) {
    ledState = !ledState;
    lastLedToggle = millis();
    Serial.println(ledState ? "LED yellow ON" : "LED yellow OFF");
  }
}

// -------------------- OTA Version Check --------------------
void checkForUpdate() {
  Serial.println("📡 Checking for OTA update...");

  HTTPClient http;
  http.begin(versionUrl);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String newVersion = http.getString();
    newVersion.trim();
    Serial.printf("🔄 Current: %s | Latest: %s\n", currentVersion.c_str(), newVersion.c_str());

    if (newVersion != currentVersion) {
      Serial.println("⬆️ New version found! Starting OTA update...");
      performOTA();
    } else {
      Serial.println("✅ Firmware is already up-to-date.");
    }
  } else {
    Serial.printf("❌ Version fetch failed. HTTP code: %d\n", httpCode);
  }

  http.end();
}

// -------------------- Perform OTA --------------------
void performOTA() {
  WiFiClientSecure client;
  client.setInsecure();

  Serial.println("📥 Downloading firmware...");
  t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("❌ OTA Failed. Error: %s\n", httpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("📭 No new firmware found.");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("✅ OTA Successful! Rebooting...");
      break;
  }
}

// -------------------- Time Sync (NTP) --------------------
void syncTime() {
  configTime(19800, 0, "pool.ntp.org", "time.nist.gov");

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("⚠️ Failed to sync time.");
  } else {
    Serial.println("🕒 Time synced successfully.");
  }
}
