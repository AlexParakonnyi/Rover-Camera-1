#ifndef NETWORK_H
#define NETWORK_H

#include <WiFi.h>

// Настройки Wi-Fi
extern const char *ssid;
extern const char *password;
extern IPAddress localIP;
extern IPAddress localGateway;
extern IPAddress subnet;

// Инициализация Wi-Fi
bool initWiFi();

#endif