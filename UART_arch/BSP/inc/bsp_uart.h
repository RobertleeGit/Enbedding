#ifndef __BSP_UART_H
#define __BSP_UART_H

/************************ Include *************************/
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "queue.h"
#include "usart.h"
#include "elog.h"
#include "semphr.h"

#include "circular_buffer.h"
/************************ Include *************************/



/************************ Defines *************************/
#define BUFFER_SIZE		    100
#define SEND_TO_BACKEND     0x1A1B1C1D
extern QueueHandle_t        uart_to_backend_Queue;
/************************ Defines *************************/



/************************ Declare *************************/
void uart_frontendTask_Function(void *argument);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
/************************ Declare *************************/

#endif /*__BSP_UART_H */
