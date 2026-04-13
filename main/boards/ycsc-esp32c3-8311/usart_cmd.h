// 串口命令头文件



#ifndef __USART_CMD_H__
#define __USART_CMD_H__

#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"


//串口命令定义，命令格式为帧头0x55 0x52 + 命令字 +命令 + 帧尾
#define USART_CMD_HEAD1 0x55
#define USART_CMD_HEAD2 0x52
#define USART_CMD_TAIL  0x5B //帧尾



//命令字定义
// CI1302 --> ESP32C3
// ESP32C3接收命令字定义


//语音唤醒词命令
#define U_CMD_CI_TO_C3_WAKE_UP 0xA0
// 语音打断命令
#define U_CMD_CI_TO_C3_VOICE_BREAK 0xA1
//语音控制命令
#define U_CMD_CI_TO_C3_VOICE_CTRL 0xA2
//按键按下命令
#define U_CMD_CI_TO_C3_KEY_DOWN 0xA3





#endif // __USART_CMD_H__