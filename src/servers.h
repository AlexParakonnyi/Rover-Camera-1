#ifndef SERVERS_H
#define SERVERS_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>

// Синхронный сервер
extern WebSocketsServer webSocket;

extern AsyncWebServer serverCameraController;

// Функции
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void streamTask(void *parameter);
void initServer();

#endif