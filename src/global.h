#ifndef GLOBAL_H
#define GLOBAL_H

#include <Arduino.h>

// Общие глобальные переменные
extern volatile int8_t cameraClientId;
extern SemaphoreHandle_t frameSemaphore;

#endif