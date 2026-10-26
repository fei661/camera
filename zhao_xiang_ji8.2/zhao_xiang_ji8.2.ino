/**********************************************************************
  Filename    : Camera and SDcard
  Description : Use the onboard buttons to take photo and save them to an SD card.
  Auther      : www.freenove.com
  Modification: 2022/11/02
**********************************************************************/
#include "esp_camera.h"
#define CAMERA_MODEL_ESP32S3_EYE
#include "camera_pins.h"
#include "ws2812.h"
#include "sd_read_write.h"
#include <TFT_eSPI.h>
#include "tft_display.h"

#define BUTTON_PIN  0

void setup() {
  // 禁用看门狗防止重启
  disableCore0WDT();
  disableCore1WDT();
  
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  Serial.println();
  Serial.println("=== ESP32-S3 摄像头实时显示系统 ===");
  Serial.println("显示分辨率优化：QQVGA (160x120) - 参考初始版本");
  Serial.println("内存优化：使用DRAM进行实时显示，提高访问速度");
  Serial.println("性能优化：无限制帧率，双缓冲，简化图像处理");
  
  // 检查PSRAM状态（ESP32-S3的PSRAM是自动初始化的）
  Serial.println("检查PSRAM状态...");
  Serial.printf("PSRAM总大小: %d bytes\n", ESP.getPsramSize());
  Serial.printf("可用PSRAM: %d bytes\n", ESP.getFreePsram());
  
  // 检查PSRAM是否可用
  if (ESP.getPsramSize() == 0) {
    Serial.println("警告：PSRAM不可用，将使用DRAM模式");
  } else {
    Serial.println("PSRAM可用，将使用PSRAM进行实时显示");
  }
  Serial.printf("psramFound(): %s\n", psramFound() ? "true" : "false");
  
  if (psramFound()) {
    Serial.println("✅ PSRAM检测成功！");
  } else {
    Serial.println("❌ PSRAM检测失败！");
  }
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // 初始化TFT显示
  Serial.println("初始化TFT显示...");
  tftInit();
  
  ws2812Init();
  sdmmcInit();
  //removeDir(SD_MMC, "/camera");
  createDir(SD_MMC, "/camera");
  listDir(SD_MMC, "/camera", 0);
  
  Serial.println("初始化摄像头...");
  if(cameraSetup()==1){
    ws2812SetColor(2);
    Serial.println("摄像头初始化成功");
  }
  else{
    ws2812SetColor(1);
    Serial.println("摄像头初始化失败");
    return;
  }
  
  Serial.println("=== 系统初始化完成，开始实时显示 ===");
}

// 全局状态变量
bool isCapturing = false;
unsigned long captureStartTime = 0;

void loop() {
  static int frameCount = 0;
  
  // 如果正在拍照，跳过实时显示
  if (isCapturing) {
    return;
  }
  
  // 简化帧计数，减少性能开销
  frameCount++;
  
  // 获取摄像头图像（实时显示用低分辨率RGB565）
  camera_fb_t * fb = esp_camera_fb_get();
  if (fb != NULL) {
    // 检查图像格式
    if (fb->format == PIXFORMAT_RGB565) {
      // 显示实时图像
      displayCameraFrame(fb);
      
      // 每300帧显示一次统计信息（参考初始版本）
      if (frameCount % 300 == 0) {
        Serial.printf("实时显示帧数: %d, 图像: %dx%d\n", 
                      frameCount, fb->width, fb->height);
      }
    } else {
      // 格式错误 - 减少输出频率
      if (frameCount % 100 == 0) {
        Serial.printf("图像格式错误: 期望RGB565, 实际: %d\n", fb->format);
      }
    }
    
    // 立即释放帧缓冲区，避免内存泄漏
    esp_camera_fb_return(fb);
  } else {
    // 显示错误信息 - 减少输出频率
    if (frameCount % 100 == 0) {
      Serial.println("摄像头错误 - 无法获取图像缓冲区");
    }
  }
  
  // 按键拍照功能
  if(digitalRead(BUTTON_PIN)==LOW){
    delay(20);
    if(digitalRead(BUTTON_PIN)==LOW){
      // 设置拍照状态，停止实时显示
      isCapturing = true;
      captureStartTime = millis();
      
      ws2812SetColor(3); // 红色表示拍照中
      
      // 显示拍照提示
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_WHITE);
      tft.drawString("Capturing...", 50, 100);
      
      while(digitalRead(BUTTON_PIN)==LOW); // 等待按键释放
      
      // 重新配置摄像头为高分辨率JPEG模式
      Serial.println("切换到高分辨率拍照模式...");
      
      // 先反初始化摄像头
      esp_camera_deinit();
      delay(100); // 减少等待时间
      
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
      config.pin_reset = RESET_GPIO_NUM;
      config.pin_pwdn = PWDN_GPIO_NUM;
      config.xclk_freq_hz = 20000000;
      config.pixel_format = PIXFORMAT_JPEG; // 拍照用JPEG格式
      config.frame_size = FRAMESIZE_QSXGA;   // 直接使用QSXGA最高分辨率
      config.grab_mode = CAMERA_GRAB_LATEST; // 获取最新帧
      config.fb_location = CAMERA_FB_IN_PSRAM; // 使用PSRAM
      config.jpeg_quality =5;              // 超高质量
      config.fb_count = 1;                  // 单缓冲
      
      Serial.println("✅ QSXGA超高清配置: 2592x1944");
      
      // 重新初始化摄像头
      esp_err_t err = esp_camera_init(&config);
      if (err != ESP_OK) {
        Serial.printf("拍照模式摄像头初始化失败: %s\n", esp_err_to_name(err));
        tft.fillScreen(TFT_RED);
        tft.setTextColor(TFT_WHITE);
        tft.drawString("摄像头初始化失败", 20, 100);
        tft.drawString("请检查摄像头连接", 20, 130);
        delay(2000);
        isCapturing = false;
        return;
      }
      
        // 重新设置传感器参数
        sensor_t *sensor = esp_camera_sensor_get();
        if (sensor != NULL) {
          setupCameraSensor(sensor);
        }
       
       // 拍照
      camera_fb_t * fb = esp_camera_fb_get();
      if (fb != NULL) {
        Serial.printf("拍照成功: %dx%d, 大小: %d bytes\n", 
                      fb->width, fb->height, fb->len);
        tft.fillScreen(TFT_BLUE);
        tft.setTextColor(TFT_WHITE);
        tft.drawString("Saveing...", 50, 100);
        int photo_index = readFileNum(SD_MMC, "/camera");
        if(photo_index != -1) {
          String path = "/camera/" + String(photo_index) + ".jpg";
          writejpg(SD_MMC, path.c_str(), fb->buf, fb->len);
          
          // 显示拍照成功信息
          tft.fillScreen(TFT_GREEN);
          tft.setTextColor(TFT_WHITE);
          tft.drawString("Photo Saved!", 50, 80);
          tft.drawString("File: " + String(photo_index) + ".jpg", 20, 120);
          tft.drawString("Size: " + String(fb->len) + " bytes", 20, 150);
          delay(1000);
        } else {
          tft.fillScreen(TFT_RED);
          tft.drawString("SD Card Error!", 50, 100);
          delay(1000);
        }
        esp_camera_fb_return(fb);
      } else {
        Serial.println("拍照失败 - 无法获取图像缓冲区");
        tft.fillScreen(TFT_RED);
        tft.drawString("Capture Failed!", 50, 100);
        delay(1000);
      }
      
      // 重新配置摄像头为实时显示模式
      Serial.println("切换回实时显示模式...");
      
      // 先反初始化摄像头
      esp_camera_deinit();
      delay(100); // 减少等待时间
      
      config.pixel_format = PIXFORMAT_RGB565; // 实时显示用RGB565
      config.frame_size = FRAMESIZE_QQVGA;     // 160x120分辨率（参考初始版本）
      config.fb_location = CAMERA_FB_IN_DRAM; // 使用DRAM提高速度
      config.fb_count = 2;                     // 双缓冲提高帧率
      
      err = esp_camera_init(&config);
      if (err != ESP_OK) {
        Serial.printf("实时显示模式摄像头初始化失败: %s\n", esp_err_to_name(err));
      } else {
        // 重新设置传感器参数
        sensor_t *sensor = esp_camera_sensor_get();
        if (sensor != NULL) {
          setupCameraSensor(sensor);
        }
      }
      
      // 恢复实时显示
      isCapturing = false;
      ws2812SetColor(2); // 绿色表示正常
      
      Serial.printf("拍照完成，耗时: %lu ms\n", millis() - captureStartTime);
    }
  }
}

int cameraSetup(void) {
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
  config.frame_size = FRAMESIZE_QQVGA;  // 160x120 - 实时显示分辨率（参考初始版本）
  config.pixel_format = PIXFORMAT_RGB565; // 直接输出RGB565格式
  config.grab_mode = CAMERA_GRAB_LATEST;   // 获取最新帧
  
  // 使用DRAM进行实时显示，提高访问速度（参考初始版本）
  config.fb_location = CAMERA_FB_IN_DRAM;  // 使用DRAM提高实时显示速度
  config.fb_count = 2;  // 双缓冲提高帧率
  Serial.println("摄像头配置：DRAM模式，双缓冲，160x120 RGB565（优化版）");
  
  config.jpeg_quality = 12; // 实时显示不需要JPEG质量设置
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return 0;
  }

  sensor_t * s = esp_camera_sensor_get();
  if (s == NULL) {
    Serial.println("无法获取摄像头传感器");
    return 0;
  }
  
  // 设置传感器参数
  setupCameraSensor(s);

  Serial.println("Camera configuration complete!");
  return 1;
}

// 简化的传感器参数设置函数 - 基础配置
void setupCameraSensor(sensor_t *sensor) {
  if (sensor == NULL) {
    Serial.println("传感器对象为空，无法设置参数");
    return;
  }
  
  Serial.println("设置摄像头基础参数...");
  
  // 基本图像参数设置
  sensor->set_vflip(sensor, 1);      // 垂直翻转
  sensor->set_hmirror(sensor, 0);    // 禁用水平镜像
  sensor->set_brightness(sensor, 0); // 亮度 (0-2) - 默认值
  sensor->set_saturation(sensor, 2); // 饱和度 (-2-2)
  sensor->set_contrast(sensor, 0);   // 对比度 (-2-2)
  sensor->set_whitebal(sensor, 1);   // 启用白平衡
  
  // 自动曝光亮度调节设置（兼容版本）
//  sensor->set_aec2(sensor, 1);       // 启用自动曝光控制 (AEC)
  sensor->set_ae_level(sensor, -2);   // 自动曝光补偿级别 (-2到2)
//  sensor->set_aec_value(sensor, 600); // 手动曝光值 (0-1200)
  
  Serial.println("✅ 摄像头基础参数设置完成");
}
