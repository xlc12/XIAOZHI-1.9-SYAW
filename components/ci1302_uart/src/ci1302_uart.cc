#include "ci1302_uart.h"
#include <string.h>

// 全局回调函数
static ci1302_wakeup_cb_t s_wakeup_cb = NULL;
static ci1302_cmd_cb_t s_cmd_cb = NULL;
static ci1302_audio_cb_t s_audio_cb = NULL;
static ci1302_status_cb_t s_status_cb = NULL;

// 串口接收任务
static void ci1302_uart_rx_task(void *arg) {
    uint8_t *buf = (uint8_t *)malloc(CI1302_UART_BUF_SIZE);
    assert(buf != NULL);

    while (1) {
        // 读取串口数据
        int len = uart_read_bytes(CI1302_UART_PORT, buf, CI1302_UART_BUF_SIZE, pdMS_TO_TICKS(100));
        if (len > 0) {
            // 解析帧头（示例：帧头0xAA 0x55，需匹配CI1302协议）
            for (int i = 0; i < len - 4; i++) {
                if (buf[i] == 0xAA && buf[i+1] == 0x55) {
                    uint8_t data_type = buf[i+2];
                    uint16_t data_len = (buf[i+3] << 8) | buf[i+4];
                    uint8_t *data_body = buf + i + 5;
                    // 校验（示例：简单异或校验，需匹配CI1302协议）
                    uint8_t checksum = 0;
                    for (int j = 0; j < data_len + 5; j++) {
                        checksum ^= buf[i + j];
                    }
                    if (checksum != buf[i + 5 + data_len]) {
                        continue; // 校验失败，跳过
                    }

                    // 根据数据类型处理
                    switch (data_type) {
                        case CI1302_DATA_WAKEUP:
                            if (s_wakeup_cb && data_len == 2) {
                                uint16_t wakeup_id = (data_body[0] << 8) | data_body[1];
                                s_wakeup_cb(wakeup_id);
                            }
                            break;
                        case CI1302_DATA_CMD:
                            if (s_cmd_cb && data_len == 2) {
                                uint16_t cmd_id = (data_body[0] << 8) | data_body[1];
                                s_cmd_cb(cmd_id);
                            }
                            break;
                        case CI1302_DATA_AUDIO:
                            if (s_audio_cb && data_len > 0) {
                                s_audio_cb(data_body, data_len);
                            }
                            break;
                        case CI1302_DATA_STATUS:
                            if (s_status_cb && data_len == 1) {
                                s_status_cb((ci1302_status_t)data_body[0]);
                            }
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }
    free(buf);
    vTaskDelete(NULL);
}

// 初始化CI1302串口
esp_err_t ci1302_uart_init(const ci1302_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // 保存回调函数
    s_wakeup_cb = config->wakeup_cb;
    s_cmd_cb = config->cmd_cb;
    s_audio_cb = config->audio_cb;
    s_status_cb = config->status_cb;

    // 配置串口参数
    uart_config_t uart_config = {
        .baud_rate = CI1302_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    ESP_ERROR_CHECK(uart_param_config(CI1302_UART_PORT, &uart_config));
    // 设置引脚