#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

// 🔐 WiFi credentials
const char* ssid = "Airtel_Lithium_India";
const char* password = "Lithium@677781";

// 🌐 GitHub URLs for version check and firmware download
const char* versionURL  = "https://raw.githubusercontent.com/tejaschaudhari457/dummyRepo/main/ota/version.txt";
const char* firmwareURL = "https://raw.githubusercontent.com/tejaschaudhari457/dummyRepo/main/ota/firmware.bin";

// 🏷️ Local firmware version
const String currentVersion = "1.0.0";

// ✅ Forward declaration
void checkForOTAUpdate();

void setup() {
  Serial.begin(115200);
  delay(1000);

  // ✅ Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected ✅");

  // ✅ Check for OTA update
  checkForOTAUpdate();
}

void loop() {
  // 📟 Simulated LED toggle
  Serial.println("Blue LED ON ");
  delay(500);
  Serial.println("Blue LED OFF");
  delay(1500);
}

// 🔄 OTA Update Function (with progress display)
void checkForOTAUpdate() {
  Serial.println("Checking for OTA update... 🔍");

  HTTPClient http;
  http.begin(versionURL);              // ✅ Step 1: Get latest version string
  int httpCode = http.GET();

  if (httpCode == 200) {
    String latestVersion = http.getString();
    latestVersion.trim();

    Serial.println("Latest version: " + latestVersion);
    Serial.println("Current version: " + currentVersion);

    if (latestVersion != currentVersion) {
      Serial.println("New version available! Starting OTA... 🔄");

      http.end(); // close version request
      http.begin(firmwareURL); // ✅ Step 2: Download firmware binary
      int code = http.GET();

      if (code == 200) {
        int contentLength = http.getSize();
        bool canBegin = Update.begin(contentLength);

        if (canBegin) {
          WiFiClient& client = http.getStream();
          uint8_t buff[128] = { 0 };
          int written = 0;
          unsigned long lastPrint = 0;

          // ✅ Step 3: Write firmware in chunks with progress
          while (http.connected() && written < contentLength) {
            size_t available = client.available();
            if (available) {
              int read = client.readBytes(buff, ((available > sizeof(buff)) ? sizeof(buff) : available));
              Update.write(buff, read);
              written += read;

              // ✅ Show % progress every 500ms or at end
              unsigned long now = millis();
              if (now - lastPrint > 500 || written == contentLength) {
                int percent = (written * 100) / contentLength;
                Serial.printf("Update Progress: %d%%\r\n", percent);
                lastPrint = now;
              }
            }
            delay(1); // Yield to prevent watchdog timeout
          }

          // ✅ Finalize update
          if (Update.end()) {
            Serial.println("\nOTA Update complete ✅");

            if (Update.isFinished()) {
              Serial.println("Firmware updated successfully. Rebooting... 🔁");
              ESP.restart();
            } else {
              Serial.println("Update did not finish properly ❌");
            }

          } else {
            Serial.println("Update error: " + String(Update.getError()));
          }

        } else {
          Serial.println("Not enough space to begin OTA ❌");
        }

      } else {
        Serial.println("Failed to download firmware. HTTP code: " + String(code));
      }

    } else {
      Serial.println("Device is already up to date ✅");
    }

  } else {
    Serial.println("Failed to fetch version info ❌ (HTTP code: " + String(httpCode) + ")");
  }

  http.end(); // ✅ Clean up
}
