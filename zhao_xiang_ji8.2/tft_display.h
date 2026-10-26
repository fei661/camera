#ifndef __TFT_DISPLAY_H
#define __TFT_DISPLAY_H

#include "Arduino.h"
#include <TFT_eSPI.h>
#include "esp_camera.h"

// TFT屏幕配置
#define TFT_WIDTH  240
#define TFT_HEIGHT 240

// 图像缩放配置 - 优化为QQVGA分辨率
#define CAMERA_WIDTH  160
#define CAMERA_HEIGHT 120
#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 240

// TFT显示相关函数声明
void tftInit(void);
void displayCameraFrame(camera_fb_t *fb);
void scaleImage(uint16_t *src_buffer, uint16_t *dst_buffer, int src_width, int src_height, int dst_width, int dst_height);
void rotateImage90(uint16_t *src_buffer, uint16_t *dst_buffer, int width, int height);

// 全局TFT对象声明
extern TFT_eSPI tft;

#endif