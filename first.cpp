#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <time.h>

// -------------------- WiFi Configuration --------------------
// Replace with your WiFi credentials
const char *ssid = "Airtel_Lithium_India";
const char *password = "Lithium@677781";

// -------------------- OTA URLs --------------------
// Version file URL (contains latest version number as text)
const char *versionUrl = "https://raw.githubusercontent.com/tejaschaudhari457/OTA/main/ota/version.txt";

// Compiled firmware binary (compiled .bin file)
const char *firmwareUrl = "https://raw.githubusercontent.com/tejaschaudhari457/OTA/main/ota/firmware.bin";

// -------------------- Firmware Info --------------------
// Current firmware version running on the device
String currentVersion = "1.0.0";

// -------------------- OTA Check Control --------------------
// Flag to ensure OTA is checked only once after WiFi reconnects
bool checkedAfterConnect = false;

// -------------------- LED Simulation --------------------
// Used to simulate LED ON/OFF messages via Serial (no real GPIO used)
bool ledState = false;
unsigned long lastLedToggle = 0;
const unsigned long ledInterval = 1000;  // 1 second interval

// -------------------- Function Declarations --------------------
void checkForUpdate();   // Check version.txt and compare with currentVersion
void performOTA();       // If update needed, download and apply firmware
void syncTime();         // Get NTP time for secure HTTPS requests

// -------------------- Setup Function --------------------
void setup() {
  Serial.begin(115200);
  Serial.println("🔌 Booting...");

  // Start WiFi connection
  WiFi.begin(ssid, password);
  Serial.print("📶 Connecting to WiFi");

  // Wait until connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // WiFi connected
  Serial.println("\n✅ Connected to WiFi!");
  Serial.print("📡 IP Address: ");
  Serial.println(WiFi.localIP());

  // DNS resolution test for GitHub
  IPAddress githubIP;
  if (WiFi.hostByName("raw.githubusercontent.com", githubIP)) {
    Serial.print("🔗 DNS OK: ");
    Serial.println(githubIP);
  } else {
    Serial.println("❌ DNS Lookup Failed!");
  }

  // Sync NTP time to allow HTTPS certificate validation (even if skipped here)
  syncTime();

  // Check for OTA update once after connecting to WiFi
  checkForUpdate();
  checkedAfterConnect = true;
}

// -------------------- Loop Function --------------------
void loop() {
  // If WiFi reconnects after disconnect, recheck OTA
  if (WiFi.status() == WL_CONNECTED && !checkedAfterConnect) {
    Serial.println("🔁 WiFi Reconnected. Checking OTA...");
    checkForUpdate();
    checkedAfterConnect = true;
  }

  // If WiFi disconnects, reset flag so OTA check runs again on next connect
  if (WiFi.status() != WL_CONNECTED) {
    checkedAfterConnect = false;
  }

  // Simulate LED blink every second using serial logs
  if (millis() - lastLedToggle >= ledInterval) {
    ledState = !ledState;
    lastLedToggle = millis();
    Serial.println(ledState ? "💡 LED RED ON" : "🔴 LED RED OFF");
  }
}

// -------------------- OTA Version Check --------------------
void checkForUpdate() {
  Serial.println("📡 Checking for OTA update...");

  HTTPClient http;
  http.begin(versionUrl);  // Begin HTTP GET on version.txt
  int httpCode = http.GET();

  if (httpCode == 200) {
    // Success, get the version from response
    String newVersion = http.getString();
    newVersion.trim(); // Remove spaces/newlines
    Serial.printf("🔄 Current: %s | Latest: %s\n", currentVersion.c_str(), newVersion.c_str());

    // If the version is different, proceed with OTA
    if (newVersion != currentVersion) {
      Serial.println("⬆️ New version found! Starting OTA update...");
      performOTA();
    } else {
      Serial.println("✅ Firmware is already up-to-date.");
    }
  } else {
    // Failed to fetch version.txt
    Serial.printf("❌ Version fetch failed. HTTP code: %d\n", httpCode);
  }

  http.end();  // End HTTP session
}

// -------------------- Perform OTA Update --------------------
void performOTA() {
  // Secure client for HTTPS (setInsecure disables certificate validation)
  WiFiClientSecure client;
  client.setInsecure();  // WARNING: Not secure. Use real certificate in production.

  // Start OTA update from firmware.bin URL
  t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl);

  // Handle OTA result
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("❌ OTA Failed. Error: %s\n", httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("📭 No new firmware found.");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("✅ OTA Successful! Rebooting now...");
      // ESP32 will auto-reboot after OTA if successful
      break;
  }
}

// -------------------- Time Sync (NTP) --------------------
void syncTime() {
  // Set NTP servers and time zone (GMT+5:30 = 19800 sec)
  configTime(19800, 0, "pool.ntp.org", "time.nist.gov");

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("⚠️ Failed to sync time from NTP.");
  } else {
    Serial.println("🕒 Time synced successfully.");
  }
}
