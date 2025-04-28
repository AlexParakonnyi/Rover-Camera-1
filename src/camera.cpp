#include "camera.h"

SemaphoreHandle_t frameSemaphore;

// Инициализация камеры
void setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  //   config.xclk_freq_hz = 30000000;
  config.pixel_format = PIXFORMAT_JPEG;
  // config.frame_size = FRAMESIZE_SVGA;  // 800x600
  config.frame_size = FRAMESIZE_XGA;  // 1024x768
  config.jpeg_quality = 10;
  config.fb_count = 2;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;

  frameSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(frameSemaphore);

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    ESP.restart();
  }

  sensor_t *s = esp_camera_sensor_get();

  // Проверяем текущие настройки камеры
  Serial.printf("1 Camera PID: 0x%x\n", s->id.PID);
  Serial.printf("1 Initial framesize: %d\n", s->status.framesize);
  Serial.printf("1 Pixel format: %d\n", s->pixformat);

  s->set_vflip(s, 0);
  s->set_hmirror(s, 0);
  s->set_brightness(s, -1);
  s->set_contrast(s, 1);
  s->set_saturation(s, 2);
  s->set_colorbar(s, 0);
  s->set_whitebal(s, 1);
  s->set_gain_ctrl(s, 1);
  s->set_exposure_ctrl(s, 1);
  s->set_aec2(s, 1);
  s->set_ae_level(s, 2);
  s->set_lenc(s, 1);
  s->set_raw_gma(s, 1);
  s->set_dcw(s, 1);
  s->set_bpc(s, 1);
  s->set_wpc(s, 1);
  s->set_sharpness(s, 1);
  s->set_res_raw(s, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, false, false);
  // s->set_quality(s, 10);
  s->set_quality(s, 8);
  //   s->set_framesize(s, FRAMESIZE_SVGA);  // Размер кадра SVGA (800x600) - 8
  //   - 9
  s->set_framesize(s, FRAMESIZE_XGA);  // Размер кадра XGA (1024x768) - 4
  // s->set_framesize(s, FRAMESIZE_HD); // Размер кадра HD (1280x720) - 5
  // s->set_framesize(s, FRAMESIZE_SXGA); // Размер кадра SXGA (1280x1024) - 6

  s->set_special_effect(s, 0);
  //   s->set_fb_count(s, 2);

  Serial.println("Camera initialized");
  // Проверяем текущие настройки камеры
  Serial.printf("2 Camera PID: 0x%x\n", s->id.PID);
  Serial.printf("2 Initial framesize: %d\n", s->status.framesize);
  Serial.printf("2 Pixel format: %d\n", s->pixformat);
}

String getStatusFramesize() {
  sensor_t *s = esp_camera_sensor_get();
  return String(s->status.framesize);
}

framesize_t setFramesize(framesize_t framesize) {
  sensor_t *s = esp_camera_sensor_get();
  s->set_framesize(s, framesize);
  return s->status.framesize;
}

void setupFramesize(int framesize) {
  sensor_t *s = esp_camera_sensor_get();
  if (framesize == 0) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  } else if (framesize == 1) {
    s->set_framesize(s, FRAMESIZE_CIF);
  } else if (framesize == 2) {
    s->set_framesize(s, FRAMESIZE_VGA);
  } else if (framesize == 3) {
    s->set_framesize(s, FRAMESIZE_SVGA);
  } else if (framesize == 4) {
    s->set_framesize(s, FRAMESIZE_XGA);
  } else if (framesize == 5) {
    s->set_framesize(s, FRAMESIZE_HD);
  } else if (framesize == 6) {
    s->set_framesize(s, FRAMESIZE_SXGA);
  }
}

camera_fb_t *getCameraFrame() {
  if (xSemaphoreTake(frameSemaphore, pdMS_TO_TICKS(1000)) ==
      pdTRUE) {  // Увеличиваем таймаут до 1 сек
    // Принудительно сбрасываем буфер перед захватом
    esp_camera_fb_get();  // Захватываем "пустой" кадр для синхронизации
    esp_camera_fb_return(esp_camera_fb_get());
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Failed to get frame");
      xSemaphoreGive(frameSemaphore);
      return NULL;
    }
    return fb;
  }
  Serial.println("Failed to take semaphore");
  return NULL;
}

void returnCameraFrame(camera_fb_t *fb) {
  if (fb) {
    esp_camera_fb_return(fb);
    xSemaphoreGive(frameSemaphore);
  }
}
