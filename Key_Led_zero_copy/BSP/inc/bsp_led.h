#ifndef __BSP_LED_H
#define __BSP_LED_H

//************************** Include ********************************//

#include "stm32f411xe.h"
#include "gpio.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "queue.h"
#include "bsp_irq_key.h"

//************************** Include ********************************//

//************************** Defines ********************************//

typedef enum
{
    LED_OK      = 0,        // Operation completed successfully
    LED_ERROR   = 1,        // Run-time error without case matched
} led_status_t;


typedef enum
{
    ON      = 0,
    OFF     = 1,
    TOGGLE  = 2
} led_operation_t;



extern QueueHandle_t key_led_Queue;

//************************** Defines ********************************//

//************************* Declaring *******************************//

led_status_t LED(led_operation_t operation);

void LED_Task(void *argument);
void LED_TIM2_Callback(void);
void LED_StartBlink(uint8_t times);
void LED_StopBlink(void);

//************************* Declaring *******************************//

#endif /* __BSP_LED_H */
