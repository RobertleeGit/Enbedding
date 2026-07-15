#ifndef __UART_PRASE_TASK_H
#define __UART_PRASE_TASK_H

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "queue.h"
#include "bsp_uart.h"
#include "elog.h"
#include "string.h"

typedef enum {
    IDLE = 0,
    RECV
} uart_task_status_t;

#define START_OF_FRAME      0xFE
#define END_OF_FRAME        0xFF
#define MAX_FRAME_LEN       100

extern circular_buffer_t cb;

void uart_backendTask_Function(void *argument);

#endif /*__UART_PRASE_TASK_H */
