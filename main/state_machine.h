#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "device_state.h"
#include <mutex>
#include <esp_log.h>

#define STATE_MACHINE_TAG "StateMachine"

class StateMachine {
public:
    enum class Event {
        NONE,
        START_LISTENING,
        START_SPEAKING,
        STOP_SPEAKING,
        STOP_LISTENING,
        WAKE_WORD_DETECTED,
        AUDIO_RESPONSE_RECEIVED,
        ABORT_SPEAKING
    };

    StateMachine() : current_state_(kDeviceStateUnknown) {}

    // 检查状态转换是否合法
    bool IsLegalTransition(DeviceState from, DeviceState to) const {
        switch (from) {
            case kDeviceStateUnknown:
                return to == kDeviceStateStarting;
                
            case kDeviceStateStarting:
                return to == kDeviceStateIdle || to == kDeviceStateWifiConfiguring || 
                       to == kDeviceStateActivating;
                
            case kDeviceStateIdle:
                return to == kDeviceStateConnecting || to == kDeviceStateListening || 
                       to == kDeviceStateAudioTesting || to == kDeviceStateUpgrading ||
                       to == kDeviceStateActivating;
                
            case kDeviceStateConnecting:
                return to == kDeviceStateListening || to == kDeviceStateIdle;
                
            case kDeviceStateListening:
                return to == kDeviceStateSpeaking || to == kDeviceStateIdle || 
                       to == kDeviceStateConnecting;
                
            case kDeviceStateSpeaking:
                return to == kDeviceStateListening || to == kDeviceStateIdle;
                
            case kDeviceStateWifiConfiguring:
                return to == kDeviceStateAudioTesting || to == kDeviceStateIdle;
                
            case kDeviceStateAudioTesting:
                return to == kDeviceStateWifiConfiguring || to == kDeviceStateIdle;
                
            case kDeviceStateActivating:
                return to == kDeviceStateIdle;
                
            case kDeviceStateUpgrading:
                return to == kDeviceStateIdle; // 升级后应该重启，这里作为安全处理
                
            default:
                return false;
        }
    }

    // 根据事件获取目标状态
    DeviceState GetTargetState(DeviceState current, Event event) const {
        switch (current) {
            case kDeviceStateIdle:
                if (event == Event::START_LISTENING) {
                    return kDeviceStateListening;
                } else if (event == Event::START_SPEAKING) {
                    return kDeviceStateSpeaking;
                }
                break;
                
            case kDeviceStateListening:
                if (event == Event::START_SPEAKING || event == Event::AUDIO_RESPONSE_RECEIVED) {
                    return kDeviceStateSpeaking;
                } else if (event == Event::STOP_LISTENING) {
                    return kDeviceStateIdle;
                } else if (event == Event::WAKE_WORD_DETECTED) {
                    return kDeviceStateConnecting; // 重新连接
                }
                break;
                
            case kDeviceStateSpeaking:
                if (event == Event::STOP_SPEAKING) {
                    return kDeviceStateListening; // 默认回到监听状态
                } else if (event == Event::ABORT_SPEAKING) {
                    return kDeviceStateIdle;
                }
                break;
                
            default:
                break;
        }
        return current; // 默认不改变状态
    }

    // 线程安全的状态设置
    bool SetState(DeviceState new_state) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (current_state_ == new_state) {
            return true; // 状态未改变
        }
        
        if (!IsLegalTransition(current_state_, new_state)) {
            ESP_LOGW(STATE_MACHINE_TAG, "Illegal state transition from %d to %d", 
                     static_cast<int>(current_state_), static_cast<int>(new_state));
            return false;
        }
        
        ESP_LOGI(STATE_MACHINE_TAG, "State transition: %d -> %d", 
                 static_cast<int>(current_state_), static_cast<int>(new_state));
        
        previous_state_ = current_state_;
        current_state_ = new_state;
        return true;
    }

    DeviceState GetCurrentState() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return current_state_;
    }

    DeviceState GetPreviousState() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return previous_state_;
    }

private:
    mutable std::mutex mutex_;
    DeviceState current_state_;
    DeviceState previous_state_;
};

#endif // STATE_MACHINE_H