// #include "wifi_board.h"
#include "dual_network_board.h"
#include "codecs/es8311_audio_codec.h"
#include "display/lcd_display.h"
// #include "font_awesome_symbols.h"
#include <font_awesome.h>
#include "application.h"
#include "button.h"
#include "config.h"
#include "mcp_server.h"
#include "settings.h"

#include <wifi_station.h>
#include <esp_log.h>
#include <esp_lcd_panel_vendor.h>
#include <driver/i2c_master.h>
#include <driver/spi_common.h>
#include <driver/uart.h>
#include <cstring>

#include "led/circular_strip.h"

#include "assets/lang_config.h"

#include "esp32_camera.h"

#include "i2c_device.h"
// 添加头文件包含
#include "boards/common/mpu6050.h"


#include "play_controller.h"

#include "boards/common/da218e.h"

#include "gsensor_action.h"

#include "driver/touch_pad.h"

#include "power_manager.h"


#if defined(LCD_TYPE_ILI9341_SERIAL)
#include "esp_lcd_ili9341.h"
#endif

#if defined(LCD_TYPE_GC9A01_SERIAL)
#include "esp_lcd_gc9a01.h"
static const gc9a01_lcd_init_cmd_t gc9107_lcd_init_cmds[] = {
    //  {cmd, { data }, data_size, delay_ms}
    {0xfe, (uint8_t[]){0x00}, 0, 0},
    {0xef, (uint8_t[]){0x00}, 0, 0},
    {0xb0, (uint8_t[]){0xc0}, 1, 0},
    {0xb1, (uint8_t[]){0x80}, 1, 0},
    {0xb2, (uint8_t[]){0x27}, 1, 0},
    {0xb3, (uint8_t[]){0x13}, 1, 0},
    {0xb6, (uint8_t[]){0x19}, 1, 0},
    {0xb7, (uint8_t[]){0x05}, 1, 0},
    {0xac, (uint8_t[]){0xc8}, 1, 0},
    {0xab, (uint8_t[]){0x0f}, 1, 0},
    {0x3a, (uint8_t[]){0x05}, 1, 0},
    {0xb4, (uint8_t[]){0x04}, 1, 0},
    {0xa8, (uint8_t[]){0x08}, 1, 0},
    {0xb8, (uint8_t[]){0x08}, 1, 0},
    {0xea, (uint8_t[]){0x02}, 1, 0},
    {0xe8, (uint8_t[]){0x2A}, 1, 0},
    {0xe9, (uint8_t[]){0x47}, 1, 0},
    {0xe7, (uint8_t[]){0x5f}, 1, 0},
    {0xc6, (uint8_t[]){0x21}, 1, 0},
    {0xc7, (uint8_t[]){0x15}, 1, 0},
    {0xf0,
    (uint8_t[]){0x1D, 0x38, 0x09, 0x4D, 0x92, 0x2F, 0x35, 0x52, 0x1E, 0x0C,
                0x04, 0x12, 0x14, 0x1f},
    14, 0},
    {0xf1,
    (uint8_t[]){0x16, 0x40, 0x1C, 0x54, 0xA9, 0x2D, 0x2E, 0x56, 0x10, 0x0D,
                0x0C, 0x1A, 0x14, 0x1E},
    14, 0},
    {0xf4, (uint8_t[]){0x00, 0x00, 0xFF}, 3, 0},
    {0xba, (uint8_t[]){0xFF, 0xFF}, 2, 0},
};
#endif // LCD_TYPE_GC9A01_SERIAL

#define TAG "YcscEsp32s3Lcd8311Ml307"

#define AUDIO_INPUT_SAMPLE_RATE  16000//24000
#define AUDIO_OUTPUT_SAMPLE_RATE 16000//24000

LV_FONT_DECLARE(font_puhui_16_4);
LV_FONT_DECLARE(font_awesome_16_4);



class YcscEs8311AudioCodec : public Es8311AudioCodec {
private:    

public:
    YcscEs8311AudioCodec(void* i2c_master_handle, i2c_port_t i2c_port, int input_sample_rate, int output_sample_rate,
                        gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din,
                        gpio_num_t pa_pin, uint8_t es8311_addr, bool use_mclk = true)
        : Es8311AudioCodec(i2c_master_handle, i2c_port, input_sample_rate, output_sample_rate,
                             mclk,  bclk,  ws,  dout,  din,pa_pin,  es8311_addr,  use_mclk = true) {}

    void EnableOutput(bool enable) override {
        if (enable == output_enabled_) {
            return;
        }
        if (enable) {
            Es8311AudioCodec::EnableOutput(enable);
        } else {
           // Nothing todo because the display io and PA io conflict
        }
    }
};

class YcscEsp32s3Lcd8311Ml307 : public DualNetworkBoard {
private:
    

    Button boot_button_;
    Button right_button_;
    Button left_button_;
    Button touch_button_;


    Display* display_;
    Esp32Camera* camera_;

    i2c_master_bus_handle_t i2c_bus_;

    // i2c_master_bus_handle_t i2c_bus_mpu6050_;
    // Mpu6050* mpu6050_;
    
    Da218e* da218e_;
    i2c_master_bus_handle_t i2c_bus_da218e_;


    Play_Controller* play_controller = nullptr;

    GsensorAction* gsensor_action_;



    uint32_t touch_value = 0;

    PowerManager* power_manager_;

    uint32_t is_long_press= 0;

    uint32_t is_enable_aec = 0;


    uint32_t last_touch_time_ = 0;
    static const uint32_t TOUCH_DEBOUNCE_INTERVAL_MS = 7000; // 设置防抖间隔为1秒



    void InitializePowerManager() {
        power_manager_ =
            new PowerManager(POWER_CHARGE_DETECT_PIN, POWER_ADC_UNIT, POWER_ADC_CHANNEL);
    }



    void touch_init() {
        touch_pad_init();
        touch_pad_config(TOUCH_PAD_NUM8); // 配置 GPIO4 为触摸引脚
        touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER); // 设置 FSM 模式为定时器模式
        touch_pad_fsm_start();
        vTaskDelay(40 / portTICK_PERIOD_MS);
        
    }

    static void touch_read_task(void* arg) {
        int aaa = 0;
        int is_enable_touch = 0;
        YcscEsp32s3Lcd8311Ml307* self = static_cast<YcscEsp32s3Lcd8311Ml307*>(arg);
        auto& app = Application::GetInstance();
        while (1) {
            touch_pad_read_raw_data(TOUCH_PAD_NUM8, &self->touch_value);

            
            if (self->touch_value > 35000 && is_enable_touch == 0) 
            {
                /* code */
                is_enable_touch = 1;
                app.SendSensorData("touch-hand", "stop", "The ");
    
                vTaskDelay(pdMS_TO_TICKS(50));
                app.SendSensorData("touch-hand", "start", "");
                ESP_LOGI(TAG, "Touch pad 8666666: %lu, ", self->touch_value);
            }

            is_enable_touch++;
            if(is_enable_touch > 60) {
                is_enable_touch = 0;
            }

            aaa++;
            if(aaa > 30) {
                aaa = 0;
                ESP_LOGI(TAG, "Touch pad 8: %lu, ", self->touch_value);
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
    



    void InitializeI2c() {
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
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus_));
    }

    void InitializeI2cBusMpu6050() {
        i2c_master_bus_config_t i2c_bus_config = {
            .i2c_port = I2C_NUM_1,
            .sda_io_num = DA218E_I2C_SDA_PIN,
            .scl_io_num = DA218E_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = true,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bus_da218e_));
    }

    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_MOSI_PIN;
        buscfg.miso_io_num = GPIO_NUM_NC;
        buscfg.sclk_io_num = DISPLAY_CLK_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    void InitializeButtons() {
        // boot_button_.OnClick([this]() {
 
        //     auto& app = Application::GetInstance();
        //     if (GetNetworkType() == NetworkType::WIFI) {
        //         if (app.GetDeviceState() == kDeviceStateStarting && !WifiStation::GetInstance().IsConnected()) {
        //             // cast to WifiBoard
        //             auto& wifi_board = static_cast<WifiBoard&>(GetCurrentBoard());
        //             wifi_board.ResetWifiConfiguration();
        //         }
        //     }
        //     app.ToggleChatState();
        // });

        // boot_button_.OnLongPress([this]() {
        //     SwitchNetworkType();
        // });

        //按下一次唤醒，再按一次则休眠，如果在说话，则打断；
        // right_button_.OnPressDown([this]() {
        //     auto& app = Application::GetInstance();
        //     app.ToggleChatState();
        //     ESP_LOGI(TAG, "66666666666666  111111111Right button clicked");
           
        // });


        
        #if !CONFIG_USE_ESP_TOUCH //V2使用ESP触摸，V3使用GPIO触摸按钮
    
       //触摸
       touch_button_.OnPressDown([this]() {
            auto& app = Application::GetInstance();
            uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (current_time - last_touch_time_ < COOLING_TIME_MS) {
                return; // 如果距离上次触发时间小于间隔，直接返回
            }
            last_touch_time_ = current_time;
            //发送传感器消息-begin
            app.SendSensorData("touch-hand", "stop", "The ");
    
            vTaskDelay(pdMS_TO_TICKS(100));
            app.SendSensorData("touch-hand", "start", "");
            //发送传感器消息-end

            ESP_LOGI(TAG, "aaaaaaaaaaaa11111111111  touch_button_  OnPressUp");
        });

        #endif
    

        //左按钮。
        left_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (GetNetworkType() == NetworkType::WIFI) {
                if (app.GetDeviceState() == kDeviceStateStarting && !WifiStation::GetInstance().IsConnected()) {
                    // cast to WifiBoard
                    auto& wifi_board = static_cast<WifiBoard&>(GetCurrentBoard());
                    wifi_board.ResetWifiConfiguration();
                }
            }
            if(is_long_press == 0){
                app.ToggleChatState();
                ESP_LOGI(TAG, "1111111aaaaaaa  left_button_  OnClick");
            } else {
                is_long_press = 0;
                app.StopListening();
                // app.PlaySound(Lang::Sounds::OGG_POPUP);
                ESP_LOGI(TAG, "222222222aaaaaa  left_button_  OnClick");
            }
            
            ESP_LOGI(TAG, " 333333333aaaaa left_button_  OnClick");

        });


        left_button_.OnDoubleClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateSpeaking) {
                // 如果当前状态是Speaking，停止Speaking
                app.ToggleChatState();
                vTaskDelay(pdMS_TO_TICKS(100));
                ESP_LOGI(TAG, "111111  kDeviceStateSpeaking  OnDoubleClick - StopSpeaking");
      
            } 
            if (app.GetDeviceState() == kDeviceStateListening) {
                app.ToggleChatState();
                vTaskDelay(pdMS_TO_TICKS(100));
                ESP_LOGI(TAG, "111111  kDeviceStateListening  OnDoubleClick - StopListening");
            }
            //延时100ms
            vTaskDelay(pdMS_TO_TICKS(100));
            
            is_long_press = 1;
            app.StartListening();
            vTaskDelay(pdMS_TO_TICKS(100));
            // app.PlaySound(Lang::Sounds::OGG_POPUP);
            // if(app.GetAecMode() == kAecOff) {
            //     // app.StartListening();
            //     ESP_LOGI(TAG, "111111111111111111111111111111111  left_button_  StartListening");
            // }
            ESP_LOGI(TAG, "22222222222222222222  left_button_  OnDoubleClick");

            
            
            
        });

        // left_button_.OnPressUp([this]() {
        //     auto& app = Application::GetInstance();
        //     if(is_long_press == 1) {
        //         is_long_press = 0;
        //         app.StopListening();
        //         ESP_LOGI(TAG, "333333333  left_button_  StopListening");
                
        //     } else {
        //         // ESP_LOGI(TAG, "4444444  left_button_  ToggleChatState");
        //         // app.ToggleChatState();
        //     }

        //     ESP_LOGI(TAG, "5555555555  left_button_  OnPressUp");
            
        // });



        #if CONFIG_USE_DEVICE_AEC
        left_button_.OnLongPress([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateIdle) {
                app.SetAecMode(app.GetAecMode() == kAecOff ? kAecOnDeviceSide : kAecOff);
                if(app.GetAecMode() == kAecOff )
                {
        
                    app.PlaySound(Lang::Sounds::OGG_POPUP);
                    app.PlaySound(Lang::Sounds::OGG_POPUP);
                }else if(app.GetAecMode() == kAecOnDeviceSide )
                {
        
                    app.PlaySound(Lang::Sounds::OGG_POPUP);
                    //延时
                    vTaskDelay(pdMS_TO_TICKS(100));
                    app.PlaySound(Lang::Sounds::OGG_POPUP);
                    app.PlaySound(Lang::Sounds::OGG_POPUP);
                }
            }
            ESP_LOGI(TAG, "66666666666666  left_button_  OnMultipleClick 3 times");
        });
        #endif

        //切换网络模式
        left_button_.OnMultipleClick([this]() {
            ESP_LOGI(TAG, "66666666666666  left_button_  OnMultipleClick 7 times");
            SwitchNetworkType();
            
           
        },5);

        
    }

    void InitializeLcdDisplay() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;
        // 液晶屏控制IO初始化
        ESP_LOGD(TAG, "Install panel IO");
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = DISPLAY_SPI_MODE;
        io_config.pclk_hz = 40 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI3_HOST, &io_config, &panel_io));

        // 初始化液晶屏驱动芯片
        ESP_LOGD(TAG, "Install LCD driver");
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RST_PIN;
        panel_config.rgb_ele_order = DISPLAY_RGB_ORDER;
        panel_config.bits_per_pixel = 16;


#if defined(LCD_TYPE_ILI9341_SERIAL)
        ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(panel_io, &panel_config, &panel));
#elif defined(LCD_TYPE_GC9A01_SERIAL)
        ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(panel_io, &panel_config, &panel));
        gc9a01_vendor_config_t gc9107_vendor_config = {
            .init_cmds = gc9107_lcd_init_cmds,
            .init_cmds_size = sizeof(gc9107_lcd_init_cmds) / sizeof(gc9a01_lcd_init_cmd_t),
        };        
#else
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));
#endif
        
        esp_lcd_panel_reset(panel);
 

        esp_lcd_panel_init(panel);
        esp_lcd_panel_invert_color(panel, DISPLAY_INVERT_COLOR);
        esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY);
        esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
#ifdef  LCD_TYPE_GC9A01_SERIAL
        panel_config.vendor_config = &gc9107_vendor_config;
#endif
        display_ = new SpiLcdDisplay(panel_io, panel,
                                    DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY,
                                    {
                                        .text_font = &font_puhui_16_4,
                                        .icon_font = &font_awesome_16_4,
#if CONFIG_USE_WECHAT_MESSAGE_STYLE
                                        .emoji_font = font_emoji_32_init(),
#else
                                        .emoji_font = DISPLAY_HEIGHT >= 240 ? font_emoji_64_init() : font_emoji_32_init(),
#endif
                                    });
    }

    



public:
    YcscEsp32s3Lcd8311Ml307() : 
    DualNetworkBoard(ML307_TX_PIN, ML307_RX_PIN,GPIO_NUM_NC),
    boot_button_(BOOT_BUTTON_GPIO),
    right_button_(RIGHT_BUTTON_GPIO),
    left_button_(LEFT_BUTTON_GPIO),
    touch_button_(TOUCH_BUTTON_GPIO)
     {

        // audio_player = new SimpleOggPlayer();
        InitializeI2c();
        InitializeSpi();
        InitializeLcdDisplay();
        InitializeButtons();


        InitializeI2cBusMpu6050();
        da218e_ = new Da218e(i2c_bus_da218e_, DA218E_DEFAULT_ADDR);

        gsensor_action_ = new GsensorAction(i2c_bus_da218e_, DA218E_DEFAULT_ADDR);

        // play_controller = new Play_Controller();

        #if CONFIG_USE_ESP_TOUCH //V2使用ESP触摸
    
        touch_init();
        xTaskCreate(touch_read_task, "touch_read_task", 2048, this, 5, NULL);

        #endif

        InitializePowerManager();
       


    }

    virtual AudioCodec* GetAudioCodec() override {
         static YcscEs8311AudioCodec audio_codec(i2c_bus_, I2C_NUM_0, AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK, AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN,
            AUDIO_CODEC_PA_PIN, AUDIO_CODEC_ES8311_ADDR);
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }


    virtual Led* GetLed() override {
        static CircularStrip led(BUILTIN_LED_GPIO, 3);
        return &led;
    }

    virtual bool GetBatteryLevel(int& level, bool& charging, bool& discharging) override {
        charging = power_manager_->IsCharging();
        discharging = !charging;
        level = power_manager_->GetBatteryLevel();
        return true;
    }


};

DECLARE_BOARD(YcscEsp32s3Lcd8311Ml307);