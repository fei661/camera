#ifndef __OV5640_AUTOFOCUS_H
#define __OV5640_AUTOFOCUS_H

#include "Arduino.h"
#include "esp_camera.h"

// 编译时控制自动对焦功能
// 注释掉下面这行可以禁用自动对焦功能，保持与定焦版本的兼容性
#define ENABLE_AUTOFOCUS

#ifdef ENABLE_AUTOFOCUS

// OV5640 自动对焦相关寄存器定义
#define OV5640_AUTO_FOCUS_CMD_REG          0x3022
#define OV5640_AUTO_FOCUS_STATUS_REG       0x3023
#define OV5640_AUTO_FOCUS_CTRL_REG         0x3024
#define OV5640_AUTO_FOCUS_WIN_CTRL_REG     0x3025
#define OV5640_AUTO_FOCUS_WIN_START_X_REG   0x3026
#define OV5640_AUTO_FOCUS_WIN_START_Y_REG   0x3027
#define OV5640_AUTO_FOCUS_WIN_END_X_REG     0x3028
#define OV5640_AUTO_FOCUS_WIN_END_Y_REG     0x3029

// 自动对焦命令定义
#define AF_CMD_IDLE         0x00
#define AF_CMD_START        0x02
#define AF_CMD_CANCEL       0x03
#define AF_CMD_FINISH       0x04
#define AF_CMD_RELEASE      0x05

// 自动对焦状态定义
#define AF_STATUS_IDLE      0x00
#define AF_STATUS_BUSY      0x01
#define AF_STATUS_SUCCESS   0x02
#define AF_STATUS_FAILED    0x03

// 自动对焦模式定义
typedef enum {
    AF_MODE_SINGLE = 0,     // 单次自动对焦
    AF_MODE_CONTINUOUS = 1  // 连续自动对焦
} af_mode_t;

// 自动对焦状态枚举
typedef enum {
    AF_STATE_IDLE = 0,      // 空闲状态
    AF_STATE_FOCUSING = 1,  // 对焦中
    AF_STATE_SUCCESS = 2,   // 对焦成功
    AF_STATE_FAILED = 3     // 对焦失败
} af_state_t;

// 自动对焦配置结构体
typedef struct {
    bool enabled;           // 是否启用自动对焦
    af_mode_t mode;         // 自动对焦模式
    uint16_t timeout_ms;    // 对焦超时时间（毫秒）
    uint8_t retry_count;    // 重试次数
} af_config_t;

// 自动对焦函数声明
bool initAutofocus(sensor_t *sensor);
bool triggerAutofocus(sensor_t *sensor, af_mode_t mode = AF_MODE_SINGLE);
af_state_t getAutofocusStatus(sensor_t *sensor);
bool waitForAutofocus(sensor_t *sensor, uint16_t timeout_ms = 3000);
bool isAutofocusSupported(sensor_t *sensor);
void setAutofocusConfig(const af_config_t *config);
af_config_t getAutofocusConfig(void);

// 内部辅助函数声明
bool writeOV5640Register(sensor_t *sensor, uint16_t reg, uint8_t value);
uint8_t readOV5640Register(sensor_t *sensor, uint16_t reg);
bool detectAutofocusCapability(sensor_t *sensor);

#else
// 当禁用自动对焦时，提供空实现以保持兼容性
#define initAutofocus(sensor) true
#define triggerAutofocus(sensor, mode) true
#define getAutofocusStatus(sensor) AF_STATE_IDLE
#define waitForAutofocus(sensor, timeout) true
#define isAutofocusSupported(sensor) false
#define setAutofocusConfig(config) 
#define getAutofocusConfig() {false, AF_MODE_SINGLE, 3000, 3}
#endif // ENABLE_AUTOFOCUS

#endif // __OV5640_AUTOFOCUS_H
