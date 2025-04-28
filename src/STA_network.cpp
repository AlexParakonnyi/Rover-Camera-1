#include "STA_network.h"

#include "secrets.h"

// Настройки Wi-Fi (объявление)
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
IPAddress localIP(192, 168, 4, 2);
IPAddress localGateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 0, 0);

// Инициализация Wi-Fi
bool initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.config(localIP, localGateway, subnet);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  uint32_t startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > 10000) {
      Serial.println("Failed to connect to WiFi");
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println(WiFi.localIP());
  return true;
}
