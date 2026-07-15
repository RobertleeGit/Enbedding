#ifndef __BSP_UART_H
#define __BSP_UART_H

/************************ Include *************************/
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "queue.h"
#include "usart.h"
#include "elog.h"
#include "semphr.h"
#include "string.h"

#include "circular_buffer.h"
/************************ Include *************************/



/************************ Defines *************************/
#define UART_IT_RECEVICE    0
#define UART_DMA_RECEVICE   1

#define BUFFER_SIZE		    10
#define SEND_TO_FRONTEND    0x0A0B0C0D
#define SEND_TO_BACKEND     0x1A1B1C1D

extern QueueHandle_t        uart_irq_Queue;
extern QueueHandle_t        uart_to_backend_Queue;
/************************ Defines *************************/



/************************ Declare *************************/
void uart_frontendTask_Function(void *argument);

#if UART_IT_RECEVICE
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
#endif
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);
/************************ Declare *************************/

#endif /*__BSP_UART_H */
