#ifndef __BSP_ADC_DMA_H
#define __BSP_ADC_DMA_H

/************************** Include *******************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "queue.h"
#include "semphr.h"

/************************** Include *******************************/


/************************** Defines *******************************/

#define BUFFER_SIZE       1


extern ADC_HandleTypeDef hadc1;
extern osThreadId_t adc_dmaTaskHandle;
extern QueueHandle_t Queue1;
extern SemaphoreHandle_t xMutex;
/************************** Defines *******************************/


/************************** Declares *******************************/

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);
void adc_dmaTask(void *argument);

/************************** Declares *******************************/

#endif /* __BSP_ADC_DMA_H */
