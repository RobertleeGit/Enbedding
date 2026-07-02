#ifndef __BSP_KEY_H
#define __BSP_KEY_H

//************************** Include ********************************//

#include "stm32f411xe.h"
#include "gpio.h"

//************************** Include ********************************//

//************************** Defines ********************************//

// 按键事件类型
typedef enum {
    KEY_EVENT_NONE = 0,                     // 没有事件
    KEY_EVENT_SHORT_PRESS,                  // 短按事件
    KEY_EVENT_LONG_PRESS                    // 长按事件
} keyevent_t;

// 状态枚举
typedef enum {
    KEY_RELEASE,                            // 按键释放
    KEY_PRESS_CONFIRM,                      // 按键按下确认
    KEY_PRESS,                              // 按键按下
    KEY_RELEASE_CONFIRM                     // 按键释放确认
} keystate_t;

typedef struct {
    keystate_t   key_state;                 // 当前状态
    keyevent_t   key_event;                 // 当前事件
    uint32_t     press_time;                // 按键按下的持续时间
    uint32_t     confirm_time;              // 消抖确认时间
} KeyContext_t;

#define DEBOUNCE_TIME           2           // 防抖动时间 20 tick
#define KEY_LONG_PRESS_TIME     60         // 长按时间阈值 60 tick

//************************** Defines ********************************//

//************************* Declaring *******************************//

void Key_Init(KeyContext_t *ctx);
void Key_Scan(KeyContext_t *ctx, keyevent_t *event);

//************************* Declaring *******************************//

#endif /* __BSP_KEY_H */
