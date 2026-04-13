#include "wifi_board.h"
#include "codecs/es8311_audio_codec.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "led/circular_strip.h"
#include "led_strip_control.h"
#include "usart_cmd.h"

#include <wifi_station.h>
#include <esp_log.h>
#include <esp_efuse_table.h>
#include <driver/i2c_master.h>
#include <driver/uart.h>

#define TAG "YcscEsp32C3Lcd8311"

class YcscEsp32C3Lcd8311 : public WifiBoard {
private:
    i2c_master_bus_handle_t codec_i2c_bus_;
    Button boot_button_;
    CircularStrip* led_strip_;

    void InitializeCodecI2c() {
        // Initialize I2C peripheral
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &codec_i2c_bus_));

        // Print I2C bus info
        if (i2c_master_probe(codec_i2c_bus_, 0x18, 1000) != ESP_OK) {
            while (true) {
                ESP_LOGE(TAG, "Failed to probe I2C bus, please check if you have installed the correct firmware");
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
        }
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting && !WifiStation::GetInstance().IsConnected()) {
                ResetWifiConfiguration();
            }
            app.ToggleChatState();
        });
        boot_button_.OnPressDown([this]() {
            Application::GetInstance().StartListening();
        });
        boot_button_.OnPressUp([this]() {
            Application::GetInstance().StopListening();
        });
    }

    // 物联网初始化，添加对 AI 可见设备
    void InitializeTools() {
        led_strip_ = new CircularStrip(BUILTIN_LED_GPIO, 8);
        new LedStripControl(led_strip_);
    }


    //通信串口
    void InitSerial_2_Control()
    {
        // 配置UART参数
        uart_config_t uart_config = {
            .baud_rate = U_2_BAUD_RATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_APB,
        };

        ESP_ERROR_CHECK(uart_driver_delete(UART_2_PORT));
        
        // 安装UART驱动
        esp_err_t ret = uart_driver_install(UART_2_PORT, U_2_BUF_SIZE * 2, 0, 0, NULL, 0);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "安装UART22驱动失败: %s", esp_err_to_name(ret));
            
        }
        
        // 配置UART参数
        ret = uart_param_config(UART_2_PORT, &uart_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "配置UART22参数失败: %s", esp_err_to_name(ret));
            
        }
        
        // 设置UART引脚
        ret = uart_set_pin(UART_2_PORT, TX2_PIN, RX2_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "设置UART2引脚失败: %s", esp_err_to_name(ret));
            
        }
        
        ESP_LOGI(TAG, "串口2初始化完成，波特率：%d，TX: GPIO%d，RX: GPIO%d", 
                U_2_BAUD_RATE, TX2_PIN, RX2_PIN);
    }

  
            // 数据包结构
    struct Uart2Packet {
        uint8_t header1;  // USART_CMD_HEAD1 (0x55)
        uint8_t header2;  // USART_CMD_HEAD2 (0x52)
        uint8_t cmd;      // 命令字
        uint8_t data[8];  // 数据，最多8字节
        uint8_t data_len; // 数据长度
        uint8_t tail;     // USART_CMD_TAIL (0x5B)
    };

    TaskHandle_t uart2_task_handle_;  // 添加串口2任务句柄
    static constexpr size_t UART2_BUF_SIZE = 1024;  // 接收缓冲区大小

    TimerHandle_t heartbeat_timer_;  // 心跳定时器句柄
    static constexpr uint32_t HEARTBEAT_INTERVAL_MS = 5000;  // 心跳间隔5秒
 

   // 串口接收任务、串口解析
    static void uart2_receive_task(void* arg) {
        YcscEsp32C3Lcd8311* instance = static_cast<YcscEsp32C3Lcd8311*>(arg);
        uint8_t buffer[32];
        int buffer_index = 0;
        
        
        while (1) {
            int len = uart_read_bytes(UART_2_PORT, buffer + buffer_index, 
                                    sizeof(buffer) - buffer_index, pdMS_TO_TICKS(10));
            if (len > 0) {
                buffer_index += len;
                
                // 检查是否有完整的数据包
                while (buffer_index >= 5) { // 最小包大小：包头1+包头2+命令字+数据+包尾
                    // 查找包头
                    int packet_start = -1;
                    int packet_size = 0;
                    for (int i = 0; i <= buffer_index - 4; i++) {
                        if (buffer[i] == USART_CMD_HEAD1 && buffer[i + 1] == USART_CMD_HEAD2) {
                            // packet_start = i; // 注释掉，因为需要找到包尾来确定数据长度

                            //查找包尾
                            for (int j = i + 4; j <= buffer_index - 1; j++) {
                                if (buffer[j] == USART_CMD_TAIL) {
                                    packet_size = j - i + 1;

                                    packet_start = i;
                                    break;
                                }
                            }
                            
                            // break;
                        }
                    }
                    
                    if (packet_start == -1) {
                        // 没找到包头，保留最后3个字节
                        if (buffer_index > 3) {
                            memmove(buffer, buffer + buffer_index - 3, 3);
                            buffer_index = 3;
                        }
                        break;
                    }
                    
       
                    // 数据包格式：包头1(1) + 包头2(1) + 命令字(1) + 数据(N) + 包尾(1)
                    if (packet_start + packet_size <= buffer_index) {
                        
                                // 解析数据包
                                uint8_t cmd = buffer[packet_start + 2];
                                uint8_t* data = &buffer[packet_start + 3];
                                uint8_t data_len = packet_size - 4; // 减去包头包尾命令字
                                
                                ESP_LOGI(TAG, "Received valid packet:");
                                ESP_LOGI(TAG, "Command: 0x%02X, Data Length: %d", cmd, data_len);
                                
                                // 根据命令字处理数据
                                instance->process_uart2_command(cmd, data, data_len);
                                
                                // 移除已处理的数据
                                int remaining = buffer_index - (packet_start + packet_size);
                                if (remaining > 0) {
                                    memmove(buffer, buffer + packet_start + packet_size, remaining);
                                }
                                buffer_index = remaining;
                            
                        
                    } else {
                        // 数据包不完整，等待更多数据
                        break;
                    }
                }     
            }        
        }
    }

    // 处理串口2命令
    
    void process_uart2_command(uint8_t cmd, uint8_t* data, uint8_t data_len) {
        auto display = GetDisplay();
        auto codec = Board::GetInstance().GetAudioCodec();
        auto& app = Application::GetInstance();

        static uint32_t last_low_battery_time_ = 0; // 电池电量过低的最后时间点
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

        ////打印
        ESP_LOGI(TAG, "Received UART2 command: 0x%02X, Data Length: %d", cmd, data_len);
        switch (cmd) {
            
            
             //充电提示上报命令，数据为1表示充电中，数据为2表示拔掉充电器，数据为3表示充满
            case U_CMD_CI_TO_C3_WAKE_UP: // 语音唤醒词命令
                ESP_LOGE(TAG, "Received wake up command666666666");
                if(app.GetDeviceState() == kDeviceStateIdle)
                {
                    app.ToggleChatState();
                }
                if(app.GetDeviceState() == kDeviceStateSpeaking)
                {
                    //延时1秒，等待语音播放完成
                    vTaskDelay(600 / portTICK_PERIOD_MS);
                    app.ToggleChatState();
                }
                break;
            case U_CMD_CI_TO_C3_VOICE_BREAK: // 语音中断命令
                ESP_LOGE(TAG, "Received voice break command: 语音打断");
                if(app.GetDeviceState() == kDeviceStateSpeaking)
                {
                    vTaskDelay(600 / portTICK_PERIOD_MS);
                    app.ToggleChatState();
                }
                break;
            case U_CMD_CI_TO_C3_VOICE_CTRL: 
                ESP_LOGE(TAG, "Received voice control command: 语音控制");
                //进入待机状态
                app.SetDeviceState(kDeviceStateIdle);
                break;

            case U_CMD_CI_TO_C3_KEY_DOWN: // 按键按下命令
                ESP_LOGE(TAG, "Received voice control command: 按键按下");
                app.ToggleChatState();
                break;
            default:
                ESP_LOGW(TAG, "Unknown command: 0x%02X", cmd);
                break;
        }
    }

public:
    YcscEsp32C3Lcd8311() : boot_button_(BOOT_BUTTON_GPIO) {
        InitializeCodecI2c();
        InitializeButtons();
        InitializeTools();
        InitSerial_2_Control();

        // 创建串口2接收任务
        BaseType_t ret = xTaskCreate(
            uart2_receive_task,
            "uart2_rx",
            4096,  // 栈大小
            this,  // 参数传递this指针
            5,     // 优先级
            &uart2_task_handle_
        );
        
        // 把 ESP32C3 的 VDD SPI 引脚作为普通 GPIO 口使用
        // esp_efuse_write_field_bit(ESP_EFUSE_VDD_SPI_AS_GPIO);
    }

    virtual Led* GetLed() override {
        // return led_strip_;
        static CircularStrip led(BUILTIN_LED_GPIO, 3);
        return &led;
    }

    virtual AudioCodec* GetAudioCodec() override {
        static Es8311AudioCodec audio_codec(codec_i2c_bus_, I2C_NUM_0, AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK, AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN,
            AUDIO_CODEC_PA_PIN, AUDIO_CODEC_ES8311_ADDR);
        return &audio_codec;
    }
};

DECLARE_BOARD(YcscEsp32C3Lcd8311);