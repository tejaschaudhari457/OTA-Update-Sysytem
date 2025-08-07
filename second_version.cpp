
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

// WiFi and OTA URLs
const char* ssid = "Airtel_Lithium_India";
const char* password = "Lithium@677781";

const char* versionURL  = "https://raw.githubusercontent.com/tejaschaudhari457/dummyRepo/main/ota/version.txt";
const char* firmwareURL = "https://raw.githubusercontent.com/tejaschaudhari457/dummyRepo/main/ota/firmware.bin";

// Local firmware version
const String currentVersion = "1.0.0";

// 👇👇 Add this forward declaration before setup() 👇👇
void checkForOTAUpdate();

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected.");

  checkForOTAUpdate(); // now this is valid
}

void loop() {
  Serial.println("Blue LED ON");
  delay(500);
  Serial.println("Blue LED OFF");
  delay(1500);
}

// 👇 Full function implementation below
void checkForOTAUpdate() {
  Serial.println("Checking for OTA update...");

  HTTPClient http;
  http.begin(versionURL);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String latestVersion = http.getString();
    latestVersion.trim();

    Serial.println("Latest version: " + latestVersion);
    Serial.println("Current version: " + currentVersion);

    if (latestVersion != currentVersion) {
      Serial.println("New version available! Starting OTA...");

      http.end();
      http.begin(firmwareURL);
      int code = http.GET();

      if (code == 200) {
        int contentLength = http.getSize();
        bool canBegin = Update.begin(contentLength);

        if (canBegin) {
          WiFiClient& client = http.getStream();
          size_t written = Update.writeStream(client);

          if (written == contentLength) {
            Serial.println("Written : " + String(written) + " successfully");
          } else {
            Serial.println("Written only : " + String(written) + "/" + String(contentLength));
          }

          if (Update.end()) {
            Serial.println("OTA finished!");
            if (Update.isFinished()) {
              Serial.println("Update successful. Rebooting...");
              ESP.restart();
            } else {
              Serial.println("Update failed.");
            }
          } else {
            Serial.println("Update error: " + String(Update.getError()));
          }
        } else {
          Serial.println("Not enough space for OTA.");
        }
      } else {
        Serial.println("Failed to download firmware. HTTP code: " + String(code));
      }
    } else {
      Serial.println("Device is already up to date.");
    }
  } else {
    Serial.println("Failed to fetch version info. HTTP code: " + String(httpCode));
  }

  http.end();
}
