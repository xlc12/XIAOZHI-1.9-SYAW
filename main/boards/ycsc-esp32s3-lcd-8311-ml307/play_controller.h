/*
    Otto机器人控制器 - MCP协议版本
*/

#ifndef HI_CONTROLLER_H
#define HI_CONTROLLER_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <cJSON.h>
#include <esp_log.h>
#include <cstring>

#include "application.h"
#include "board.h"
#include "config.h"
#include "mcp_server.h"
#include "sdkconfig.h"
#include "settings.h"

#include "application.h"

#include "assets/lang_config.h"


// 前向声明
class MiniKame_hi;

class Play_Controller {
private:
   
    TaskHandle_t play_task_handle_ = nullptr;
    QueueHandle_t play_queue_;
    bool is_play_in_progress_ = false;

    struct PlayOggActionParams {
        int action_type;
        int steps;
        int speed;
        int direction;
        int amount;
    };

    enum Oggtype {
        ACTION_WALK = 1,
        ACTION_BACKWARD = 2,
        ACTION_TRUN_L = 3,
        ACTION_TRUN_R = 4,
        ACTION_HELLO = 5,
        ACTION_SWAY = 6,
        ACTION_NOD = 7,

        ACTION_HOME = 12,
        ACTION_ZERO = 13,
    };

    // 私有静态方法
    static void PlayActionTask(void* arg);
    void StartPlayActionTaskIfNeeded();
    void QueuePlayAction(int action_type, int steps, int speed, int direction, int amount);

public:
    Play_Controller();
    ~Play_Controller();
    void RegisterPlayMcpTools();
};

// 初始化函数声明
extern void InitializePlay_Controller();

#endif PLAY_CONTROLLER_H

