#include "bsp_led.h"

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
                LED(TOGGLE);
            }
            else if(KEY_EVENT_LONG_PRESS  == event)
            {
                for (size_t i = 0; i < 3; i++)
                {
                LED(ON);
                HAL_Delay(500);
                LED(OFF);
                HAL_Delay(500);
                }
            }
        }
		osDelay(100);
	}
}
