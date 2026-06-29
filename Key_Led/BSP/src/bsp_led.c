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
