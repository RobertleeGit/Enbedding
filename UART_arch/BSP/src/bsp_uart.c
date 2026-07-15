#include "bsp_uart.h"

// 环形缓冲区
circular_buffer_t cb = {0};
uint8_t rx_buff[BUFFER_SIZE] = {0};
// 临时接收数据
static uint8_t received_data = 0;
// 用于通知前端的信号量
static SemaphoreHandle_t uart_irq_sem = NULL;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	// 1. 将接收到的数据放入环形缓冲区
	if (circular_buffer_push(&cb, received_data) != BUFFER_OK) {
		log_e("Circular buffer is full! Data lost.");
	}

	// 2. 通知前端线程
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (uart_irq_sem != NULL) {
		// 中断过快，可能发送不成功
        xSemaphoreGiveFromISR(uart_irq_sem, &xHigherPriorityTaskWoken);	
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

	// 3. 开启下一次接收中断	
	if (HAL_UART_Receive_IT(huart, &received_data, 1) != HAL_OK) {
		log_e("HAL_UART_Receive_IT error!");
	}
}

void uart_frontendTask_Function(void *argument)
{
	log_i("uart_recTask create successfully!");

	if (0 == uart_to_backend_Queue) {
		log_e("Queue create failed!");
		return;
	}


	// 1. 创建信号量
    uart_irq_sem = xSemaphoreCreateBinary();
    if (uart_irq_sem == NULL) {
        log_e("Semaphore create failed!");
        return;
    }

	// 2. 初始化环形缓冲区
	circular_buffer_init(&cb, rx_buff, BUFFER_SIZE);
	
	// 3. 开启 UART 中断接收
	// HAL_UART_Receive_IT 函数直接接收数据，他只是将 g_buffer1 的指针给到硬件并开启中断，在接收到数据时由硬件自动接收数据
	if(HAL_UART_Receive_IT(&huart1, &received_data, 1) != HAL_OK) {
		log_e("HAL_UART_Receive_IT error!");
	} else {
		log_d("HAL_UART_Receive_IT OK!");
	}

	for(;;)
	{
		// 4. 阻塞等待中断通知
        if (xSemaphoreTake(uart_irq_sem, portMAX_DELAY) != pdTRUE) {
            log_e("Receive notification error!");
        }

		// 5. 通过队列通知后端
		uint32_t flag = SEND_TO_BACKEND;
		if (xQueueSend(uart_to_backend_Queue, &flag, 0)  != pdPASS) {
			log_e("front send backend error!");
		}
		
		osDelay(1);
	}
}


