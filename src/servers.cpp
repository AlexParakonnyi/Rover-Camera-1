#include "servers.h"

#include <ArduinoJson.h>

#include "camera.h"

// WebSocket сервер на порту 81 - облегченная версия для камеры
WebSocketsServer webSocket = WebSocketsServer(8081, "");

// Async server for camera control
AsyncWebServer serverCameraController(8085);

StaticJsonDocument<300> cameraJson;

// Глобальные переменные (объявление)
volatile int8_t cameraClientId = -1;

char *getFramesizeJson() {
  cameraJson.clear();  // Очищаем JSON перед заполнением
  String framesize = String(getStatusFramesize());
  cameraJson["framesize"] = framesize;
  static char
      jsonBuffer[512];  // Увеличиваем буфер, JSON большой 256 -> 512 -> 1024
  serializeJson(cameraJson, jsonBuffer);
  return jsonBuffer;
}

void handleGetFrameRequest(AsyncWebServerRequest *request) {
  // setupFramesize(2);
  AsyncWebServerResponse *response =
      request->beginResponse(200, "application/json", getFramesizeJson());
  response->addHeader("Access-Control-Allow-Origin",
                      "*");  // Разрешает запросы от всех источников
  response->addHeader("Access-Control-Allow-Methods",
                      "GET, OPTIONS");  // Разрешенные методы
  response->addHeader("Access-Control-Allow-Headers",
                      "Content-Type");  // Разрешенные заголовки
  request->send(response);
}

void handleSetFrameRequest(AsyncWebServerRequest *request) {
  framesize_t newFramesize;

  // Проверяем, есть ли параметр "size" в запросе
  if (request->hasParam("size")) {
    // Извлекаем значение параметра "size" как строку
    String sizeStr = request->getParam("size")->value();

    // Преобразуем строку в число
    int sizeValue = sizeStr.toInt();

    Serial.printf("Received size: %d\n", sizeValue);
    // Проверяем, что значение является допустимым целым числом

    // Проверяем, что значение находится в допустимом диапазоне framesize_t
    if (sizeValue >= FRAMESIZE_96X96 && sizeValue <= FRAMESIZE_UXGA) {
      newFramesize = static_cast<framesize_t>(sizeValue);
    } else {
      // Если значение некорректно, отправляем ошибку
      AsyncWebServerResponse *response = request->beginResponse(
          400, "application/json", "{\"error\": \"Invalid framesize value\"}");
      response->addHeader("Access-Control-Allow-Origin", "*");
      response->addHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
      response->addHeader("Access-Control-Allow-Headers", "Content-Type");
      request->send(response);
      return;
    }
  } else {
    // Если параметр "size" отсутствует, отправляем ошибку
    AsyncWebServerResponse *response = request->beginResponse(
        400, "application/json", "{\"error\": \"Missing size parameter\"}");
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
    request->send(response);
    return;
  }

  // Устанавливаем новый framesize
  setFramesize(newFramesize);

  // Отправляем ответ с текущим значением framesize
  AsyncWebServerResponse *response =
      request->beginResponse(200, "application/json", getFramesizeJson());
  response->addHeader("Access-Control-Allow-Origin", "*");
  response->addHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  response->addHeader("Access-Control-Allow-Headers", "Content-Type");
  request->send(response);
}

void handleImageRequest(AsyncWebServerRequest *request) {
  vTaskDelay(100 / portTICK_PERIOD_MS);
  camera_fb_t *fb = getCameraFrame();
  if (!fb) {
    AsyncWebServerResponse *response = request->beginResponse(
        500, "application/json", "{\"error\": \"Failed to capture image\"}");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    return;
  }

  Serial.printf("Captured frame: width=%d, height=%d, length=%u, format=%d\n",
                fb->width, fb->height, fb->len, fb->format);
  Serial.printf("First bytes: %02x %02x %02x %02x\n", fb->buf[0], fb->buf[1],
                fb->buf[2], fb->buf[3]);

  // Проверяем формат и конвертируем, если нужно
  uint8_t *jpg_buf = fb->buf;
  size_t jpg_len = fb->len;
  if (fb->format != PIXFORMAT_JPEG) {
    Serial.println("Converting frame to JPEG");
    bool converted = frame2jpg(fb, 80, &jpg_buf, &jpg_len);
    if (!converted) {
      Serial.println("JPEG conversion failed");
      esp_camera_fb_return(fb);
      AsyncWebServerResponse *response = request->beginResponse(
          500, "application/json", "{\"error\": \"JPEG conversion failed\"}");
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
      returnCameraFrame(fb);
      return;
    }
  }
}

void initServer() {
  serverCameraController.on("/get-framesize", HTTP_GET, handleGetFrameRequest);
  serverCameraController.on(
      "/get-framesize", HTTP_OPTIONS,
      handleGetFrameRequest);  // Обработчик OPTIONS для CORS

  serverCameraController.on("/set-framesize", HTTP_GET, handleSetFrameRequest);
  serverCameraController.on(
      "/set-framesize", HTTP_OPTIONS,
      handleSetFrameRequest);  // Обработчик OPTIONS для CORS

  serverCameraController.on("/image", HTTP_GET, handleImageRequest);
  serverCameraController.on("/image", HTTP_OPTIONS,
                            handleImageRequest);  // Обработчик OPTIONS для CORS

  serverCameraController.begin();
}

// Обработка событий WebSocket
void IRAM_ATTR onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                                size_t length) {
  if (type == WStype_CONNECTED) {
    Serial.printf("Client %d connected\n", num);
    cameraClientId = num;
  } else if (type == WStype_DISCONNECTED) {
    Serial.printf("Client %d disconnected\n", num);
    if (cameraClientId == num) {
      cameraClientId = -1;
    }
  } else if (type == WStype_TEXT) {
    // handleWebSocketMessage(num, payload, length);
    Serial.printf("We got a message: %s\n", payload);
  }
}

void streamTask(void *parameter) {
  Serial.println("Stream task started");
  TickType_t lastWakeTime = xTaskGetTickCount();

  while (true) {
    if (cameraClientId != -1) {
      camera_fb_t *fb = NULL;

      // Ожидаем семафор для получения кадра
      if (xSemaphoreTake(frameSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
        fb = esp_camera_fb_get();
        if (!fb) {
          Serial.println("Failed to get frame");
          xSemaphoreGive(frameSemaphore);
          vTaskDelay(pdMS_TO_TICKS(10));
          continue;
        }

        // Отправляем данные через WebSocket
        bool sent = webSocket.sendBIN(cameraClientId, fb->buf, fb->len);
        if (!sent) {
          Serial.println("Failed to send frame");
        }

        esp_camera_fb_return(fb);
        xSemaphoreGive(frameSemaphore);
      }

      // Адаптивная задержка для ~24 FPS
      vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(41));
    } else {
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}