#include "bsp_irq_key.h"

/**
 * @brief Redefine the HAL_GPIO_EXTI_Callback
 * @param GPIO_Pin Specifies the pins connected EXTI line
 * @return None
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == Key_Pin)
    {
        key_press_event_t evt = {0};
        evt.trigger_tick = HAL_GetTick();   // 获取当前系统 tick（单位 ms）
        
        // 读取当前电平确定方向
        if (HAL_GPIO_ReadPin(Key_GPIO_Port, Key_Pin) == GPIO_PIN_RESET)
            evt.direction = LEVEL_FALLING;
        else
            evt.direction = LEVEL_RISING;

        // 从中断中发送到队列（使用中断安全函数）
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(key_irq_Queue, &evt, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } 
}

/**
 * @brief Init the key task context
 * @param ctx: key task context
 * @return None
 */
void Key_Context_Init(key_context_t *ctx)
{
    if (ctx == NULL) return;
    ctx->press_start_tick = 0;
    ctx->key_state = KEY_RELEASE;
    ctx->key_event = KEY_EVENT_NONE;
}

/**
 * @brief Handle key interrupt events and output key event
 * @param ctx : Context of key task
 * @param evt : interrupt event
 * @param event : output key event
 * @return None
 */
void Key_Task_Function(key_context_t *ctx, key_press_event_t *press_event, keyevent_t *event)
{
    if (ctx == NULL || event == NULL) return;

    // 1. 清除本次事件标志（每次扫描开始默认无事件）
    ctx->key_event = KEY_EVENT_NONE; 

    // 2. 状态机
    switch (ctx->key_state)
    {
        // 按键释放, 只检测按下
        case KEY_RELEASE:
        {
            if (press_event->direction == LEVEL_FALLING)
            {
                ctx->press_start_tick = press_event->trigger_tick;
                ctx->key_state = KEY_PRESS;
            }
            break;
        }
        // 按键按下，等待上升沿，判断是否是正在的按键事件
        case KEY_PRESS:
        {
            if (press_event->direction == LEVEL_RISING)
            {
                uint32_t duration = press_event->trigger_tick - ctx->press_start_tick;
                // 抖动，返回释放状态
                if (duration < DEBOUNCE_TIME)
                {
                    ctx->key_state = KEY_RELEASE;
                    break;
                }
                // 有效按下事件
                if (duration > KEY_LONG_PRESS_TIME)
                {
                    ctx->key_event = KEY_EVENT_LONG_PRESS;
                }
                else
                {
                    ctx->key_event = KEY_EVENT_SHORT_PRESS;
                }
                ctx->key_state = KEY_RELEASE;
            }
            // 在按下状态中再次出现下降沿，则更新按下开始时间
            else if (press_event->direction == LEVEL_FALLING)
            {
                ctx->press_start_tick = press_event->trigger_tick;
            }
            break;
        }
        default:
        {
            ctx->key_state = KEY_RELEASE;
            break;
        }
    }
    
    // 3. 输出当前事件
    *event = ctx->key_event;
}


/**
 * @brief Task function of the Key thread
 * @param[in] argument: the pointer of the function argument 
 * @return None
 */
void Key_Task(void *argument)
{
    key_context_t key_ctx = {0};
    key_press_event_t press_event = {0};
    keyevent_t key_event = KEY_EVENT_NONE;

    Key_Context_Init(&key_ctx);

    if (key_irq_Queue == 0 || key_led_Queue == 0)
    {
    printf("The queue has not yet been created!\r\n");
    return;
    }
    else
    {
    printf("create queue successfully!\r\n");
    }

    for(;;)
	{
		// printf("KeyThread is running\r\n");
        // 1. Receive message from key_irq_Queue
        if (xQueueReceive( key_irq_Queue, &( press_event ), ( TickType_t ) 10 ))
        {
            // printf("press_event: trigger_tick: %d, direction: %s \r\n", press_event.trigger_tick, 
            //     (press_event.direction == LEVEL_RISING) ? "LEVEL_RISING" : "LEVEL_FALLING" );
            
            // // 2. Determine the events
            Key_Task_Function(&key_ctx, &press_event, &key_event);

            if (KEY_EVENT_NONE == key_event)
            {
            continue;
            }
            // 3. Send data based on valid event type
            if (key_event == KEY_EVENT_SHORT_PRESS)
                printf("Short press detected.\r\n");
            else if (key_event == KEY_EVENT_LONG_PRESS)
                printf("Long press detected.\r\n");
            
            if ( xQueueSend(key_led_Queue, &key_event, 0) != pdPASS)
            {
                printf("send message failed.\r\n");
            }
            else
            {
                printf("send message successfully.\r\n");
            }
        }
		osDelay(100);
	}
}
