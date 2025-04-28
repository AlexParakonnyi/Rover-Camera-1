#include "AP_network.h"

bool initWiFi() {
  // Пробуем создать точку доступа
  const char* ssid = "TPLINK-705";
  const char* password = "123d4567";

  // Задаём IP-адрес, шлюз и маску подсети
  IPAddress local_IP(192, 168, 1, 1);  // Желаемый IP-адрес точки доступа
  IPAddress gateway(192, 168, 1,
                    1);  // Обычно шлюз совпадает с IP точки доступа
  IPAddress subnet(255, 255, 255, 0);  // Маска подсети

  Serial.print("Configuring AP with IP: ");
  Serial.println(local_IP);

  // Настраиваем параметры точки доступа
  if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
    Serial.println("Failed to configure AP");
    while (true);  // Останавливаем выполнение, если не удалось настроить
  }

  Serial.print("Creating AP: ");
  Serial.println(ssid);

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();

  Serial.print("AP IP address: ");
  Serial.println(IP);

  bool result = (IP == local_IP);
  return result;  // Возвращаем true, если IP-адрес совпадает с ожидаемым
}