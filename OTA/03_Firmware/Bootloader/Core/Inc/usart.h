#ifndef __USART_H
#define __USART_H

#include "stm32f4xx.h"
#include <stdio.h>

// GPIO 定义
/*******************************************************/
#define USART_TX_PIN                GPIO_Pin_9                // USART TX 的 GPIO 引脚
#define USART_TX_GPIO_PORT          GPIOA                     // GPIO 端口
#define USART_TX_GPIO_CLK           RCC_AHB1Periph_GPIOA      // GPIO 端口时钟
#define USART_TX_SOURCE             GPIO_PinSource9           // GPIO 复用引脚源
#define USART_TX_AF                 GPIO_AF_USART1            // GPIO 复用为 USART1

#define USART_RX_PIN                GPIO_Pin_10               // GPIO 引脚
#define USART_RX_GPIO_PORT          GPIOA                     // GPIO 端口
#define USART_RX_GPIO_CLK           RCC_AHB1Periph_GPIOA      // GPIO 端口时钟
#define USART_RX_SOURCE             GPIO_PinSource10          // GPIO 复用引脚源
#define USART_RX_AF                 GPIO_AF_USART1            // GPIO 复用为 USART1
/*******************************************************/


// USART 定义
/*******************************************************/
#define USARTx                      USART1
#define USART_CLK                   RCC_APB2Periph_USART1           // USART 时钟
#define USART_BAUDRATE              115200                          // USART 波特率
/*******************************************************/


void USART1_Configuration(void);
void USART_SendChar(USART_TypeDef *Usartx, uint8_t data);
uint16_t USART_ReceiveChar(USART_TypeDef *Usartx);
int fputc(int ch, FILE *f);

#endif /* __USART_H */
