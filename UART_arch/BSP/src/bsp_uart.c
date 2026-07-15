#include "bsp_uart.h"

// 环形缓冲区
circular_buffer_t cb = {0};
uint8_t rx_buff[BUFFER_SIZE] = {0};

#if UART_DMA_RECEVICE
// 用于回调函数内记录上一次写偏移
static uint16_t last_write_offset = 0;
#endif

#if UART_IT_RECEVICE
// 中断临时接收数据
static uint8_t received_data = 0;
#endif

#if UART_DMA_RECEVICE	// DMA 接收数据
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart->Instance != USART1) return;

	uint32_t current_offset = Size;
	// log_d("DMA write pointer position = %d", Size);
	
	// 本次中断新增数据量
	uint16_t delta = 0;
	if (current_offset >= last_write_offset)
	{
		delta = current_offset - last_write_offset;
	} else {
		delta = cb.size - last_write_offset + current_offset;
	}

	if (delta <= 0) return;
	// log_d("Increased data volume = %d", delta);

	cb.write = (cb.write + delta) % cb.size;	// 更新缓冲区已写入的数量
	last_write_offset = current_offset;			// 更新写偏移

	// log_d("write pointer position = %d", cb.write);

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	uint32_t flag = SEND_TO_FRONTEND;
	xQueueSendFromISR(uart_irq_Queue , &flag, &xHigherPriorityTaskWoken);	
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
#endif

#if UART_IT_RECEVICE	// UART 中断接收数据
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	// 1. 将接收到的数据放入环形缓冲区
	if (circular_buffer_push(&cb, received_data) != BUFFER_OK) {
		log_e("Circular buffer is full! Data lost.");
	}

	// 2. 通知前端线程
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	// 中断过快，可能发送不成功
	uint32_t flag = SEND_TO_FRONTEND;
	xQueueSendFromISR(uart_irq_Queue , &flag, &xHigherPriorityTaskWoken);	
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

	// 3. 开启下一次接收中断	
	if (HAL_UART_Receive_IT(huart, &received_data, 1) != HAL_OK) {
		log_e("HAL_UART_Receive_IT error!");
	}
}
#endif

void uart_frontendTask_Function(void *argument)
{
	log_i("uart_recTask create successfully!");

	if (0 == uart_to_backend_Queue) {
		log_e("uart_to_backend_Queue create failed!");
		return;
	}

	if (0 == uart_irq_Queue)
	{
		log_e("uart_irq_Queue create failed!");
		return;
	}

	// 1. 初始化环形缓冲区
	memset(rx_buff, 0, BUFFER_SIZE);
	circular_buffer_init(&cb, rx_buff, BUFFER_SIZE);

#if UART_IT_RECEVICE	// UART 中断接收
	// 2. 开启 UART 中断接收
	// HAL_UART_Receive_IT 函数直接接收数据，他只是将 g_buffer1 的指针给到硬件并开启中断，在接收到数据时由硬件自动接收数据
	if(HAL_UART_Receive_IT(&huart1, &received_data, 1) != HAL_OK) {
		log_e("HAL_UART_Receive_IT error!");
	} else {
		log_d("HAL_UART_Receive_IT OK!");
	}
#endif

#if UART_DMA_RECEVICE	// UART DMA接收
	if (HAL_UARTEx_ReceiveToIdle_DMA(&huart1, cb.buffer, cb.size) != HAL_OK) {
		log_e(" HAL_UART_Receive_DMA error!");
	} else {
		log_d(" HAL_UART_Receive_DMA OK!");
	}
#endif

	for(;;)
	{
		// 阻塞等待中断通知
		uint32_t flag = 0;
        if (xQueueReceive(uart_irq_Queue, &flag, portMAX_DELAY) != pdTRUE) {
            log_e("Receive notification error!");
        }
		if (flag != SEND_TO_FRONTEND)
		{
			log_e("Invalid frontend flag!");
		}

		// 通过队列通知后端
		flag = SEND_TO_BACKEND;
		if (xQueueSend(uart_to_backend_Queue, &flag, 0)  != pdPASS) {
			log_e("front send backend error!");
		}
		osDelay(1);
	}
}


