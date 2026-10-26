#include "tft_display.h"

// 全局TFT对象
TFT_eSPI tft = TFT_eSPI();

// TFT显示缓冲区 - 优化为QQVGA尺寸
uint16_t display_buffer[DISPLAY_WIDTH * DISPLAY_HEIGHT];
// 图像旋转缓冲区 - 用于存储旋转后的图像
uint16_t rotate_buffer[CAMERA_WIDTH * CAMERA_HEIGHT];

void tftInit(void) {
  // 初始化TFT屏幕
  tft.init();
  tft.setRotation(2); // 设置屏幕方向 - 翻转180度
  tft.fillScreen(TFT_BLACK); // 清屏为黑色
  
  // 设置显示参数
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  
  // 显示初始化信息
  tft.drawString("Camera Ready", 50, 100);
  
  Serial.println("TFT display initialized successfully");
}

void displayCameraFrame(camera_fb_t *fb) {
  if (fb == NULL) {
    Serial.println("Camera frame buffer is NULL");
    return;
  }
  
  // 检查帧格式
  if (fb->format != PIXFORMAT_RGB565) {
    Serial.printf("Unsupported pixel format: %d, expected RGB565\n", fb->format);
    return;
  }
  
  // 先对摄像头图像进行90度顺时针旋转
  rotateImage90((uint16_t*)fb->buf, rotate_buffer, fb->width, fb->height);
  
  // 然后缩放旋转后的图像到显示尺寸
  scaleImage(rotate_buffer, display_buffer, fb->height, fb->width, DISPLAY_WIDTH, DISPLAY_HEIGHT);
  
  // 显示到TFT屏幕
  tft.pushImage(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, display_buffer);
}

void scaleImage(uint16_t *src_buffer, uint16_t *dst_buffer, int src_width, int src_height, int dst_width, int dst_height) {
  // 优化的最近邻插值缩放算法 - 提高性能
  float x_ratio = (float)src_width / dst_width;
  float y_ratio = (float)src_height / dst_height;
  
  for (int y = 0; y < dst_height; y++) {
    int src_y = (int)(y * y_ratio);
    if (src_y >= src_height) src_y = src_height - 1;
    
    for (int x = 0; x < dst_width; x++) {
      int src_x = (int)(x * x_ratio);
      if (src_x >= src_width) src_x = src_width - 1;
      
      // 最近邻插值 - 最快速度
      dst_buffer[y * dst_width + x] = src_buffer[src_y * src_width + src_x];
    }
  }
}

void rotateImage90(uint16_t *src_buffer, uint16_t *dst_buffer, int width, int height) {
  // 90度顺时针旋转算法
  // 旋转后：原图像的宽度变成高度，高度变成宽度
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      // 90度顺时针旋转的坐标变换：
      // 原坐标(x,y) -> 新坐标(height-1-y, x)
      int new_x = height - 1 - y;
      int new_y = x;
      
      // 将原图像像素复制到旋转后的位置
      dst_buffer[new_y * height + new_x] = src_buffer[y * width + x];
    }
  }
}