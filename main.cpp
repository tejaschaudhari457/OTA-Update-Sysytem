#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

// --- Function Declarations ---
void checkForUpdate();
void performOTA();

// --- WiFi Credentials ---
const char *ssid = "Airtel_Lithium_India";
const char *password = "Lithium@677781";

// --- GitHub Raw URLs (public repo required) ---
const char *versionUrl = "https://raw.githubusercontent.com/tejaslithium/OTA/main/ota/version.txt";
const char *firmwareUrl = "https://raw.githubusercontent.com/tejaslithium/OTA/main/ota/firmware.bin";

// --- Current Firmware Version ---
String currentVersion = "1.0.0";

// --- OTA Update Interval (12 hours) ---
unsigned long lastUpdateCheck = 0;
const unsigned long updateInterval = 12UL * 60UL * 60UL * 1000UL; // 12 hours

void setup() {
  Serial.begin(115200);
  Serial.println("Booting...");

  // Optional: set static IP and Google DNS to fix DNS lookup issues
  IPAddress local_IP(192, 168, 1, 100);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress primaryDNS(8, 8, 8, 8);
  IPAddress secondaryDNS(8, 8, 4, 4);

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("⚠️ Failed to configure static IP and DNS");
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Optional: DNS test
  IPAddress githubIP;
  if (WiFi.hostByName("raw.githubusercontent.com", githubIP)) {
    Serial.print("DNS OK: raw.githubusercontent.com resolved to ");
    Serial.println(githubIP);
  } else {
    Serial.println("❌ DNS Lookup failed!");
  }

  // First-time OTA check
  checkForUpdate();
}

void loop() {
  // Periodic OTA check
  if (millis() - lastUpdateCheck > updateInterval) {
    lastUpdateCheck = millis();
    checkForUpdate();
  }

  // Other main loop logic
  delay(1000);
}

void checkForUpdate() {
  Serial.println("📡 Checking for OTA update...");

  HTTPClient http;
  http.begin(versionUrl);

  int httpCode = http.GET();
  if (httpCode == 200) {
    String newVersion = http.getString();
    newVersion.trim();

    Serial.printf("🔄 Current: %s, Latest: %s\n", currentVersion.c_str(), newVersion.c_str());

    if (newVersion != currentVersion) {
      Serial.println("⬆️ New firmware available! Starting OTA update...");
      performOTA();
    } else {
      Serial.println("✅ Firmware is up-to-date.");
    }
  } else {
    Serial.printf("❌ Failed to fetch version file, HTTP code: %d\n", httpCode);
  }

  http.end();
}

void performOTA() {
  WiFiClientSecure client;
  client.setInsecure();  // For development (skip SSL verification)

  t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("❌ Update Failed. Error: %s\n", httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("📭 No update available.");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("✅ Update successful! Rebooting...");
      break;
  }
}
