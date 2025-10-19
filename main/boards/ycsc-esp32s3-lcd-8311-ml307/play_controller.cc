/*
    Otto机器人控制器 - MCP协议版本
*/



#include "play_controller.h"

#define TAG "Play_Controller"

/******** PlayActionTask 实现 ********/
void Play_Controller::PlayActionTask(void* arg) {
    Play_Controller* controller = static_cast<Play_Controller*>(arg);
    PlayOggActionParams params;
    auto& app = Application::GetInstance();

    while (true) {
        if (xQueueReceive(controller->play_queue_, &params, pdMS_TO_TICKS(1000)) == pdTRUE) {
            ESP_LOGI(TAG, "执行动作: %d", params.action_type);
            controller->is_play_in_progress_ = true;

            switch (params.action_type) {
                case ACTION_HOME:
                   
                    ESP_LOGI(TAG, "666666666:111111111111");
                    break;
                
                case ACTION_ZERO:
                    
                    ESP_LOGI(TAG, "666666666:22222222222"); 
                    break;

                case ACTION_WALK:
                    // app.PlaySound(Lang::Sounds::OGG_MAIDANLAOOUT);
                    ESP_LOGI(TAG, "666666666:333333333333");
                    break;

                case ACTION_BACKWARD:
                   
                    ESP_LOGI(TAG, "666666666:444444444444");
                    break;

                case ACTION_TRUN_L:
                    vTaskDelay(pdMS_TO_TICKS(10000));
                    
                    ESP_LOGI(TAG, "666666666:555555555555");

                    break;

                case ACTION_TRUN_R:
     
                    ESP_LOGI(TAG, "666666666:666666666666");
                    break;

                case ACTION_HELLO:
                    
                    ESP_LOGI(TAG, "666666666:777777777777");
                    break;

                case ACTION_SWAY:
                    
                    ESP_LOGI(TAG, "666666666:888888888888");
                    break;
                
                case ACTION_NOD:
                    
                    ESP_LOGI(TAG, "666666666:999999999999");
                    break;
            }

            controller->is_play_in_progress_ = false;
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        // ESP_LOGI(TAG, "666666666:d");
    }
}

/******** StartPlayActionTaskIfNeeded 实现 ********/
void Play_Controller::StartPlayActionTaskIfNeeded() {
    if (play_task_handle_ == nullptr) {
        xTaskCreate(PlayActionTask, "otto_action", 1024 * 3, this, configMAX_PRIORITIES - 1,
                    &play_task_handle_);
    }
}

/******** QueuePlayAction 实现 ********/
void Play_Controller::QueuePlayAction(int action_type, int steps, int speed, int direction, int amount) {
    ESP_LOGI(TAG, "动作控制: 类型=%d, 步数=%d, 速度=%d, 方向=%d, 幅度=%d", action_type, steps,
             speed, direction, amount);

    PlayOggActionParams params = {action_type, steps, speed, direction, amount};
    xQueueSend(play_queue_, &params, portMAX_DELAY);
    StartPlayActionTaskIfNeeded();
}

/******** 构造函数实现 ********/
Play_Controller::Play_Controller() {

    play_queue_ = xQueueCreate(10, sizeof(PlayOggActionParams));

    QueuePlayAction(ACTION_HOME, 1, 1000, 1, 0);  // direction=1表示复位手部

    RegisterPlayMcpTools();
}

/******** RegisterPlayMcpTools 实现 ********/
void Play_Controller::RegisterPlayMcpTools() {
    auto& mcp_server = McpServer::GetInstance();

    ESP_LOGI(TAG, "开始注册MCP工具...");

    // 基础移动动作
    mcp_server.AddTool("self.otto.action_run",
        "前进。steps: 前进步数(1-15); speed: 前进速度(50-800，数值越小越快); ",
        PropertyList({Property("steps", kPropertyTypeInteger, 3, 1, 15),
                      Property("speed", kPropertyTypeInteger, 100, 50, 800)
                     }),
        [this](const PropertyList& properties) -> ReturnValue {
            float steps = (float)properties["steps"].value<int>();
            int speed = properties["speed"].value<int>();
            QueuePlayAction(ACTION_WALK, steps, speed, 0, 0);
            return true;
        });

    mcp_server.AddTool("self.otto.action_backward",
        "后退。steps: 后退步数(1-15); speed: 后退速度(50-800，数值越小越快); ",
        PropertyList({Property("steps", kPropertyTypeInteger, 3, 1, 15),
                      Property("speed", kPropertyTypeInteger, 100, 50, 800)
                     }),
        [this](const PropertyList& properties) -> ReturnValue {
            float steps = (float)properties["steps"].value<int>();
            int speed = properties["speed"].value<int>();
            QueuePlayAction(ACTION_BACKWARD, steps, speed, 0, 0);
            return true;
        });

    mcp_server.AddTool("self.otto.action_trun_l",
        "左转。steps: 左转步数(1-10); speed: 左转速度(50-800，数值越小越快); ",
        PropertyList({Property("steps", kPropertyTypeInteger, 3, 1, 10),
                      Property("speed", kPropertyTypeInteger, 100, 50, 800)
                     }),
        [this](const PropertyList& properties) -> ReturnValue {
            float steps = (float)properties["steps"].value<int>();
            int speed = properties["speed"].value<int>();
            QueuePlayAction(ACTION_TRUN_L, steps, speed, 0, 0);
            return true;
        });

    mcp_server.AddTool("self.otto.action_trun_r",
        "右转。steps: 右转步数(1-10); speed: 右转速度(50-800，数值越小越快); ",
        PropertyList({Property("steps", kPropertyTypeInteger, 3, 1, 10),
                      Property("speed", kPropertyTypeInteger, 100, 50, 800)
                     }),
        [this](const PropertyList& properties) -> ReturnValue {
            float steps = (float)properties["steps"].value<int>();
            int speed = properties["speed"].value<int>();
            QueuePlayAction(ACTION_TRUN_R, steps, speed, 0, 0);
            return true;
        });

    mcp_server.AddTool("self.otto.action_hello",
        "打招呼。steps: 打招呼次数(1-10); speed: 打招呼速度(50-800，数值越小越快); ",
        PropertyList({Property("steps", kPropertyTypeInteger, 3, 1, 10),
                      Property("speed", kPropertyTypeInteger, 100, 50, 800)
                     }),
        [this](const PropertyList& properties) -> ReturnValue {
            float steps = (float)properties["steps"].value<int>();
            int speed = properties["speed"].value<int>();
            QueuePlayAction(ACTION_HELLO, steps, speed, 0, 0);
            return true;
        });

    mcp_server.AddTool("self.otto.action_sway",
        "摇摆。steps: 摇摆次数(1-10); speed: 摇摆速度(50-800，数值越小越快); ",
        PropertyList({Property("steps", kPropertyTypeInteger, 3, 1, 10),
                      Property("speed", kPropertyTypeInteger, 100, 50, 800)
                     }),
        [this](const PropertyList& properties) -> ReturnValue {
            float steps = (float)properties["steps"].value<int>();
            int speed = properties["speed"].value<int>();
            QueuePlayAction(ACTION_SWAY, steps, speed, 0, 0);
            return true;
        });

    mcp_server.AddTool("self.otto.action_nod",
        "点头。steps: 点头次数(1-10); speed: 点头速度(50-800，数值越小越快); ",
        PropertyList({Property("steps", kPropertyTypeInteger, 3, 1, 10),
                      Property("speed", kPropertyTypeInteger, 100, 50, 800)
                     }),
        [this](const PropertyList& properties) -> ReturnValue {
            float steps = (float)properties["steps"].value<int>();
            int speed = properties["speed"].value<int>();
            QueuePlayAction(ACTION_NOD, steps, speed, 0, 0);
            return true;
        });

    mcp_server.AddTool("self.otto.action_home",
        "复位。",
        PropertyList(),
        [this](const PropertyList& properties) -> ReturnValue {
            QueuePlayAction(ACTION_HOME, 0, 0, 0, 0);
            return true;
        });

    mcp_server.AddTool("self.otto.action_zero",
        "归零",
        PropertyList(),
        [this](const PropertyList& properties) -> ReturnValue {
            QueuePlayAction(ACTION_ZERO, 0, 0, 0, 0);
            return true;
        });

    ESP_LOGI(TAG, "MCP工具注册完成");
}

/******** 析构函数实现 ********/
Play_Controller::~Play_Controller() {
    if (play_task_handle_ != nullptr) {
        vTaskDelete(play_task_handle_);
        play_task_handle_ = nullptr;
    }
    vQueueDelete(play_queue_);
}

// 全局变量和初始化函数
static Play_Controller* play_controller = nullptr;

void InitializePlay_Controller() {
    if (play_controller == nullptr) {
        play_controller = new Play_Controller();
        ESP_LOGI(TAG, "Otto控制器已初始化并注册MCP工具");
    }
}
