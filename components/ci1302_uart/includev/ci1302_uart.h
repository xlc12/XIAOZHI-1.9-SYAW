#ifndef CI1302_UART_H
#define CI1302_UART_H

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// CI1302串口配置
#define CI1302_UART_PORT        UART_NUM_1
#define CI1302_UART_TX_PIN      17
#define CI1302_UART_RX_PIN      16
#define CI1302_UART_BAUDRATE    115200
#define CI1302_UART_BUF_SIZE    1024

// 数据类型定义
typedef enum {
    CI1302_DATA_WAKEUP = 0x01,        // 唤醒词触发
    CI1302_DATA_CMD = 0x02,           // 命令词触发
    CI1302_DATA_AUDIO = 0x03,         // 音频流数据
    CI1302_DATA_STATUS = 0x04,        // 设备状态
    CI1302_DATA_CTRL = 0x80,          // 控制命令（ESP32下发）
} ci1302_data_type_t;

// 设备状态枚举
typedef enum {
    CI1302_STATUS_IDLE = 0x00,        // 空闲
    CI1302_STATUS_WAKEUP = 0x01,      // 已唤醒
    CI1302_STATUS_CMD_DETECT = 0x02,  // 命令词检测中
    CI1302_STATUS_AUDIO_STREAM = 0x03 // 音频流传输中
} ci1302_status_t;

// 回调函数类型
typedef void (*ci1302_wakeup_cb_t)(uint16_t wakeup_id);  // 唤醒词回调
typedef void (*ci1302_cmd_cb_t)(uint16_t cmd_id);        // 命令词回调
typedef void (*ci1302_audio_cb_t)(uint8_t *data, uint16_t len); // 音频流回调
typedef void (*ci1302_status_cb_t)(ci1302_status_t status); // 状态回调

// CI1302配置结构体
typedef struct {
    ci1302_wakeup_cb_t wakeup_cb;
    ci1302_cmd_cb_t cmd_cb;
    ci1302_audio_cb_t audio_cb;
    ci1302_status_cb_t status_cb;
} ci1302_config_t;

/**
 * @brief 初始化CI1302串口通信
 * @param config 回调函数配置
 * @return esp_err_t 初始化结果
 */
esp_err_t ci1302_uart_init(const ci1302_config_t *config);

/**
 * @brief 向CI1302下发控制命令
 * @param cmd_id 命令ID
 * @param param 命令参数
 * @param param_len 参数长度
 * @return esp_err_t 发送结果
 */
esp_err_t ci1302_uart_send_ctrl_cmd(uint16_t cmd_id, uint8_t *param, uint16_t param_len);

#endif // CI1302_UART_H