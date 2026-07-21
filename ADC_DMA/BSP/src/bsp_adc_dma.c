#include "bsp_adc_dma.h"

uint32_t * buffer1 = NULL;
uint32_t * buffer2 = NULL;

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	BaseType_t *pxHigherPriorityTaskWoken = NULL;

	// ADC_DMA传输完成后，通知 adc_dmaTask 任务
	vTaskNotifyGiveFromISR(adc_dmaTaskHandle, pxHigherPriorityTaskWoken);
	// 唤醒高优先级任务
	portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
	UNUSED(hadc);
	printf("ADC transfer error!\r\n");
}


void adc_dmaTask(void *argument)
{
	if (0 == Queue1)
	{
		printf("Queue create failed!\r\n");
		return;
	}

	// 1. 分配并初始化缓存区
	buffer1 = (uint32_t *) malloc(sizeof(uint32_t) * BUFFER_SIZE);
  	buffer2 = (uint32_t *) malloc(sizeof(uint32_t) * BUFFER_SIZE);

	if (NULL == buffer1)
	{
		printf("buffer1 malloc failed\r\n");
		return;
	}
	if (NULL == buffer2)
	{
		printf("buffer2 malloc failed\r\n");
		return;
	}
  
	memset(buffer1, 0xff, ( sizeof(uint32_t) * BUFFER_SIZE ) );
	memset(buffer2, 0xff, ( sizeof(uint32_t) * BUFFER_SIZE ) );

	uint32_t * current_buf = buffer1;
	uint32_t * idle_buf = buffer2;

	// 2. 第一次触发 ADC 和 DMA 传输
	if (HAL_OK != HAL_ADC_Start_DMA(&hadc1, current_buf, BUFFER_SIZE))
	{
		printf("Failed to start ADC DMA\r\n");
		return;
	}

	for(;;)
	{
		// printf("adc_dmaTask is running!\r\n");

		// 3. 阻塞等待 DMA 传输完成中断的通知
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);		// 获取到通知后直接将其清零，无限期等待

		// 4. 通知线程 B 获取数据并处理
        if (xQueueSend(Queue1, &current_buf, portMAX_DELAY) != pdPASS) 
		{
            printf("Failed to send data to queue\r\n");
			return;
        }

		// 5. 切换传输的 buffer 指针
		uint32_t *temp = current_buf;
        current_buf = idle_buf;
        idle_buf = temp;

		// 6. 在开启下一次转换时，先获取互斥锁，保证输出线程已经处理完了
		xSemaphoreTake(xMutex, portMAX_DELAY);

		// 7. 开启 ADC_DMA 传输
		if (HAL_OK != HAL_ADC_Start_DMA(&hadc1, current_buf, BUFFER_SIZE))
		{
			printf("Failed to start ADC DMA\r\n");
			return;
		}

		// 8. 释放互斥量
		xSemaphoreGive(xMutex);
		
		osDelay(1);
	}
}

