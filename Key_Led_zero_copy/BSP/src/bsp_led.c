#include "bsp_led.h"

static volatile uint8_t s_blink_times = 0;      // The number of times the LED needs to blink
static volatile uint8_t s_blink_count = 0;      // The current blink count
static volatile uint8_t s_blink_active = 0;     // Flag indicating if blinking is active

/**
 * @brief control LED
 * @param[in] operation: LED control options.
 *      @arg LED_ON: turn on the led
 *      @arg LED_OFF: turn off the led
 *      @arg LED_TOGGLE: toggle the led level
 * @return led_status_t: Status of the function.
 */
led_status_t LED(led_operation_t operation)
{
    switch (operation)
    {
        case ON:
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
            break;
        case OFF:
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
            break;
        case TOGGLE:
            HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
            break;
        default:
            break;
    }
    return LED_OK;
}

/**
 * @brief Start blinking the LED
 * @param[in] times: the number of times the LED should blink
 * @return None
 */
void LED_StartBlink(uint8_t times)
{
    if (times == 0) return;
    // 若已有闪烁任务，先停止
    LED_StopBlink();
    s_blink_times = times;
    s_blink_count = 0;
    s_blink_active = 1;
    LED(OFF);   // 从灭开始
}

/**
 * @brief Stop blinking the LED
 * @return None
 */
void LED_StopBlink(void)
{
    s_blink_active = 0;
    s_blink_times = 0;
    s_blink_count = 0;
    LED(OFF);  // 确保熄灭
}

void LED_TIM2_Callback(void)
{
    if (s_blink_active) {
        LED(TOGGLE);                     // 每 100ms 翻转一次
        s_blink_count++;
        // 每完整闪烁一次需要 2 次切换
        if (s_blink_count >= (2 * s_blink_times)) {
            LED_StopBlink();             // 完成，熄灯并复位
        }
    }
}

/**
 * @brief Task function of the LED thread
 * @param[in] argument: the pointer of the function argument 
 * @return None
 */
void LED_Task(void *argument)
{
    keyevent_t event = KEY_EVENT_NONE;   // 事件接受变量

    if(key_led_Queue == 0)
    {
        printf("The queue has not yet been created!");
    }
    for(;;)
	{
        // printf("LEDThread is running\r\n");

        // keyQueue 中接受数据，若数据还未准备好则阻 10 tick 
        if( xQueueReceive(key_led_Queue, &(event), (TickType_t) 10) )
        {
            // 控制 LED
            printf("receive operation successfully\r\n");
            if (KEY_EVENT_SHORT_PRESS == event)
            {
                LED_StartBlink(1);
            }
            else if(KEY_EVENT_LONG_PRESS  == event)
            {
                LED_StartBlink(10);
            }
        }
		osDelay(100);
	}
}

