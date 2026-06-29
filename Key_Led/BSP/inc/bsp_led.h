#ifndef __BSP_LED_H
#define __BSP_LED_H

//************************** Include ********************************//

#include "stm32f411xe.h"
#include "gpio.h"

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

//************************** Defines ********************************//

//************************* Declaring *******************************//

led_status_t LED(led_operation_t operation);

//************************* Declaring *******************************//

#endif /* __BSP_LED_H */
