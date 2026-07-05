#ifndef __BSP_IRQ_KEY_H
#define __BSP_IRQ_KEY_H

//************************** Include ********************************//

#include "stm32f411xe.h"
#include "gpio.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "queue.h"


//************************** Include ********************************//

//************************** Defines ********************************//

typedef enum 
{
   LEVEL_RISING = 0,
   LEVEL_FALLING   
} key_level_direction_t;

typedef enum {
    KEY_EVENT_NONE = 0,
    KEY_EVENT_SHORT_PRESS,
    KEY_EVENT_LONG_PRESS
} keyevent_t;

typedef enum {
    KEY_RELEASE = 0,                       
    KEY_PRESS                                
} keystate_t;

typedef struct 
{
    uint32_t                trigger_tick;
    key_level_direction_t   direction;
} key_press_event_t;


typedef struct 
{
    uint32_t                press_start_tick;
    keystate_t              key_state;
    keyevent_t              key_event;
} key_context_t;


#define DEBOUNCE_TIME           10          // unit ms         
#define KEY_LONG_PRESS_TIME     600         // unit ms

extern QueueHandle_t key_irq_Queue;
extern QueueHandle_t key_led_Queue;

//************************** Defines ********************************//

//************************* Declaring *******************************//

void Key_Task_Function(key_context_t *ctx, key_press_event_t *press_event, keyevent_t *event);

void Key_Context_Init(key_context_t *ctx);

//************************* Declaring *******************************//

#endif /* __BSP_IRQ_KEY_H */
