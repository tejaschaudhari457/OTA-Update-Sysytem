#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <time.h>

// --- WiFi Credentials ---
const char *ssid = "Airtel_Lithium_India";
const char *password = "Lithium@677781";

// --- GitHub OTA URLs ---
const char *versionUrl = "https://raw.githubusercontent.com/tejaslithium/OTA/main/ota/version.txt";
const char *firmwareUrl = "https://raw.githubusercontent.com/tejaslithium/OTA/main/ota/firmware.bin";

// --- Current Firmware Version ---
String currentVersion = "1.0.0";

// --- OTA Interval ---
unsigned long lastUpdateCheck = 0;
const unsigned long updateInterval = 12UL * 60UL * 60UL * 1000UL; // 12 hours

// --- Time and flags ---
const long gmtOffset_sec = 19800; // GMT+5:30 for IST
bool ranMorning = false;
bool ranEvening = false;

// --- Track last WiFi status ---
wl_status_t lastWiFiStatus = WL_IDLE_STATUS;

// --- LED State ---
bool ledState = false;
unsigned long lastLedToggle = 0;
const unsigned long ledInterval = 1000; // 1 second

// --- Function declarations ---
void checkForUpdate();
void performOTA();
void syncTime();

void setup() {
  Serial.begin(115200);
  Serial.println("🔌 Booting...");

  // Optional: Static IP
  IPAddress local_IP(192, 168, 1, 100);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress primaryDNS(8, 8, 8, 8);
  IPAddress secondaryDNS(8, 8, 4, 4);
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("📶 Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to WiFi!");
  Serial.println(WiFi.localIP());

  // DNS Test
  IPAddress githubIP;
  if (WiFi.hostByName("raw.githubusercontent.com", githubIP)) {
    Serial.print("🔗 DNS OK: ");
    Serial.println(githubIP);
  } else {
    Serial.println("❌ DNS Lookup Failed!");
  }

  lastWiFiStatus = WiFi.status();

  // Sync time
  syncTime();

  // Check for update at boot
  checkForUpdate();

  // Indicate LED simulation startup
  Serial.println("ESP32 LED Simulation Started");
}

void loop() {
  // 1. Periodic OTA check every 12 hours
  if (millis() - lastUpdateCheck > updateInterval) {
    lastUpdateCheck = millis();
    checkForUpdate();
  }

  // 2. Detect WiFi reconnect
  if (WiFi.status() != lastWiFiStatus) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("🔁 WiFi reconnected, checking OTA...");
      checkForUpdate();
    }
    lastWiFiStatus = WiFi.status();
  }

  // 3. Scheduled check at 10:00 AM and 10:00 PM
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    int hour = timeinfo.tm_hour;
    int minute = timeinfo.tm_min;

    if (hour == 10 && minute == 0 && !ranMorning) {
      Serial.println("⏰ 10:00 AM — Checking for OTA update...");
      checkForUpdate();
      ranMorning = true;
    }
    if (hour == 22 && minute == 0 && !ranEvening) {
      Serial.println("⏰ 10:00 PM — Checking for OTA update...");
      checkForUpdate();
      ranEvening = true;
    }

    // Reset flags after 10:00
    if (minute != 0) {
      ranMorning = false;
      ranEvening = false;
    }
  }

  // 4. Main application logic — LED blinking
  if (millis() - lastLedToggle >= ledInterval) {
    ledState = !ledState;
    lastLedToggle = millis();

    if (ledState) {
      Serial.println("LED RED COLOR ");
    } else {
      Serial.println("LED RED COLOR ");
    }
  }
}

// --- Function to check firmware version ---
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
      Serial.println("⬆️ New version found! Performing OTA...");
      performOTA();
    } else {
      Serial.println("✅ Firmware is up-to-date.");
    }
  } else {
    Serial.printf("❌ Version fetch failed. HTTP code: %d\n", httpCode);
  }

  http.end();
}

// --- Function to perform OTA update ---
void performOTA() {
  WiFiClientSecure client;
  client.setInsecure();  // Skip certificate check

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

// --- Function to sync time from NTP ---
void syncTime() {
  configTime(gmtOffset_sec, 0, "pool.ntp.org", "time.nist.gov");
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("⚠️ Failed to obtain time from NTP");
  } else {
    Serial.println("🕒 Time synced successfully");
  }
}
