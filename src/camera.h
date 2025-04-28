#ifndef CAMERA_H
#define CAMERA_H

#include <Arduino.h>
#include <global.h>
#include <camera_pins.h>
#include <esp_camera.h>

// SemaphoreHandle_t frameSemaphore;

// Функции
void setupCamera();
camera_fb_t* getCameraFrame();
void returnCameraFrame(camera_fb_t* fb);
void setupFramesize(int framesize);
String getStatusFramesize();
framesize_t setFramesize(framesize_t framesize);

#endif