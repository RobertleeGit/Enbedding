#include "bsp_key.h"

void key_scan(key_press_status_t *key_press_value)
{
    uint32_t count = 1000;
    while (count > 0)
    {
        if (HAL_GPIO_ReadPin(Key_GPIO_Port,Key_Pin) == GPIO_PIN_RESET)
        {
            *key_press_value = KEY_PRESSED;
            return;
        }
        count--;
    }
    *key_press_value = KEY_RELEASE;
}
