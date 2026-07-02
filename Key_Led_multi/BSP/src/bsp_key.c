#include "bsp_key.h"

/**
 * @brief 初始化按键上下文
 * @param ctx: 按键上下文结构体
 */
void Key_Init(KeyContext_t *ctx)
{
    if (ctx == NULL) return;
    ctx->key_state = KEY_RELEASE;
    ctx->key_event = KEY_EVENT_NONE;
    ctx->press_time = 0;
    ctx->confirm_time = 0;
}

/**
 * @brief 可重入的按键扫描函数
 * @param ctx: 按键上下文结构体
 * @param event: 事件接收变量
 */
void Key_Scan(KeyContext_t *ctx, keyevent_t *event)
{
    if (ctx == NULL || event == NULL) return;

    // 1. 清除本次事件标志（每次扫描开始默认无事件）
    ctx->key_event = KEY_EVENT_NONE;
    
    // 2. 读取硬件电平 (0=按下, 1=释放)
    uint8_t level = HAL_GPIO_ReadPin(Key_GPIO_Port, Key_Pin);  

    // 3. 状态机
    switch (ctx->key_state)
    {
        // 按键释放
        case KEY_RELEASE:
        {
            // 检测到按键按下，转到按键按下确认
            if (level == 0) {                       
                ctx->press_time = 0;
                ctx->key_state = KEY_PRESS_CONFIRM;
            }
            break;
        }
        // 按键按下确认
        case KEY_PRESS_CONFIRM:
        {
            // 防抖时间后，确认按下，转到按键按下
            if (level == 0) {
                ctx->confirm_time++;
                if (ctx->confirm_time >= DEBOUNCE_TIME) {
                    ctx->key_state = KEY_PRESS;
                    ctx->confirm_time = 0;
                }
            } else {    // 是抖动，返回按键释放
                ctx->key_state = KEY_RELEASE;
                ctx->confirm_time = 0;
            }
            break;
        }
        // 按键按下
        case KEY_PRESS:
        {
            // 按下持续，进行计数
            if (level == 0) {
                ctx->press_time++;
            } else {    // 检测到按键释放，转到按键释放确认
                ctx->key_state = KEY_RELEASE_CONFIRM;
                ctx->confirm_time = 0;
            }
            break;
        }
        // 按键释放确认
        case KEY_RELEASE_CONFIRM:
        {
            // 释放确认成功后，进行事件判断
            if (level == 1) {
                ctx->confirm_time++;
                if (ctx->confirm_time >= DEBOUNCE_TIME) {
                    // 长按 / 短按 事件判定
                    if (ctx->press_time >= KEY_LONG_PRESS_TIME) {
                        ctx->key_event = KEY_EVENT_LONG_PRESS;
                    } else {
                        ctx->key_event = KEY_EVENT_SHORT_PRESS;
                    }
                    // 回到释放状态
                    ctx->key_state = KEY_RELEASE;
                    ctx->confirm_time = 0;
                }
            } else {    // 抖动，回到按下状态
                ctx->key_state = KEY_PRESS;
                ctx->confirm_time = 0;
            }
            break;
        }
        default:
        {
            ctx->key_state = KEY_RELEASE;
            ctx->press_time = 0;
            break;
        }
    }
    
    // 4. 输出当前事件
    *event = ctx->key_event;
}
