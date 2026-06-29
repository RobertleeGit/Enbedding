#ifndef __BSP_KEY_H
#define __BSP_KEY_H

//************************** Include ********************************//

#include "stm32f411xe.h"
#include "gpio.h"

//************************** Include ********************************//

//************************** Defines ********************************//

typedef enum
{
    KEY_PRESSED = 0,
    KEY_RELEASE = 1,
} key_press_status_t;

//************************** Defines ********************************//

//************************* Declaring *******************************//

void key_scan(key_press_status_t *key_press_value);

#endif /* __BSP_KEY_H */
