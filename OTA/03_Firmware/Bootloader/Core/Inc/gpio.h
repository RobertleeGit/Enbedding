#ifndef __gpio_H
#define __gpio_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "stm32f4xx.h"
#include "main.h"


#define Key_Pin             GPIO_Pin_0        
#define Key_GPIO_Port       GPIOA

#define LED_Pin             GPIO_Pin_13
#define LED_GPIO_Port       GPIOC

#define LED_ON              GPIO_ResetBits(LED_GPIO_Port, LED_Pin)
#define LED_OFF             GPIO_SetBits(LED_GPIO_Port, LED_Pin)

void GPIO_Config(void);
uint8_t Key_Scan(void);

#ifdef __cplusplus
}
#endif

#endif
