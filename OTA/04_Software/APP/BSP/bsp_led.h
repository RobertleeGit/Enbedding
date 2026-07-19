#ifndef __BSP_LED_H
#define __BSP_LED_H


#include "stm32f4xx.h"
#include "main.h"

typedef enum {
	ON = 0,
	OFF,
	TOGGLE
} led_operation_t;


typedef enum {
	LED_OK = 0,
	LED_ERROR
} led_status_t;

led_status_t LED(led_operation_t operation);



#endif /* __BSP_LED_H */
