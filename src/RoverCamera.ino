#include <Arduino.h>

#include "camera.h"
#include "glob.h"
#include "network.h"
#include "servers.h"

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  if (!initWiFi()) {
    Serial.println("WiFi connection failed, restarting...");
    ESP.restart();
  }

  setupCamera();

  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  Serial.println("WebSocket server started");

  initServer();

  xTaskCreatePinnedToCore(streamTask, "StreamTask", 8192, NULL, 2, NULL, 0);
}

void loop() {
  // webSocket.cleanupClients();
  webSocket.loop();
  taskYIELD();
}