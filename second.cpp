#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <time.h>

// -------------------- WiFi Configuration --------------------
const char *ssid = "Airtel_Lithium_India";
const char *password = "Lithium@677781";

// -------------------- OTA URLs --------------------
const char *versionUrl = "https://raw.githubusercontent.com/tejaschaudhari457/OTA/main/ota/version.txt";
const char *firmwareUrl = "https://raw.githubusercontent.com/tejaschaudhari457/OTA/main/ota/firmware.bin";

// -------------------- Firmware Info --------------------
String currentVersion = "1.0.1";

// -------------------- OTA Check Control --------------------
bool checkedAfterConnect = false;

// -------------------- LED Simulation (Yellow LED Logic) --------------------
bool ledState = false;
unsigned long lastToggleTime = 0;
unsigned long currentDelay = 500; // Start with LED ON duration

// -------------------- Function Declarations --------------------
void checkForUpdate();
void performOTA();
void syncTime();

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
  Serial.print("📡 IP Address: ");
  Serial.println(WiFi.localIP());

  IPAddress githubIP;
  if (WiFi.hostByName("raw.githubusercontent.com", githubIP)) {
    Serial.print("🔗 DNS OK: ");
    Serial.println(githubIP);
  } else {
    Serial.println("❌ DNS Lookup Failed!");
  }

  syncTime();
  checkForUpdate();
  checkedAfterConnect = true;
}

void loop() {
  // Recheck OTA if reconnected
  if (WiFi.status() == WL_CONNECTED && !checkedAfterConnect) {
    Serial.println("🔁 WiFi Reconnected. Checking OTA...");
    checkForUpdate();
    checkedAfterConnect = true;
  }

  // Reset flag if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    checkedAfterConnect = false;
  }

  // Yellow LED Simulation: ON 500ms, OFF 1500ms
  if (millis() - lastToggleTime >= currentDelay) {
    ledState = !ledState;
    lastToggleTime = millis();

    if (ledState) {
      Serial.println("🟡 Yellow LED ON");
      currentDelay = 500;
    } else {
      Serial.println("⚫ Yellow LED OFF");
      currentDelay = 1500;
    }
  }
}

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

void performOTA() {
  WiFiClientSecure client;
  client.setInsecure();

  t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("❌ OTA Failed. Error: %s\n", httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("📭 No new firmware found.");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("✅ OTA Successful! Rebooting now...");
      break;
  }
}

void syncTime() {
  configTime(19800, 0, "pool.ntp.org", "time.nist.gov");

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("⚠️ Failed to sync time from NTP.");
  } else {
    Serial.println("🕒 Time synced successfully.");
  }
}
