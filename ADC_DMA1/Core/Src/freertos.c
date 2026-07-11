/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>

#include "bsp_adc_dma.h"
#include "semphr.h"
#include "elog.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */


osThreadId_t adc_dmaTaskHandle;
const osThreadAttr_t adc_dmaTask_attributes = {
  .name = "ADC_DMATask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

osThreadId_t outputTaskHandle;
const osThreadAttr_t outputTask_attributes = {
  .name = "outputTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

// 用于线程 A 通知线程 B 处理数据
QueueHandle_t Queue1;


SemaphoreHandle_t xMutex = NULL;

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void outputTask(void *argument);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  xMutex = xSemaphoreCreateMutex();
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  Queue1 = xQueueCreate( 1, sizeof( uint32_t * ) );
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  // defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);
  
  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  adc_dmaTaskHandle = osThreadNew(adc_dmaTask, NULL, &adc_dmaTask_attributes);
  outputTaskHandle = osThreadNew(outputTask, NULL, &outputTask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */

  for(;;)
  {
    printf("StartDefaultTask is running!\r\n");
    osDelay(100);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void outputTask(void *argument)
{
  uint32_t * p_buf = NULL;
  double voltage = 0.0;
  
  for(;;)
  {
    // printf("outputTask is running!\r\n");

    // 1. 阻塞等待线程 A 传输的 buffer 指针 
		if (xQueueReceive(Queue1, &p_buf, portMAX_DELAY) != pdPASS) 
		{
			printf("Failed to receive data to queue\r\n");
			return;
		}

    // 2. 获取互斥量
    xSemaphoreTake(xMutex, portMAX_DELAY);

    printf("Received buffer address: %p\r\n", p_buf);

    // 3. 转换 ADC 的值为电压值
    voltage = (double)(*p_buf) * 3.3 / 4095.0;

    // 4. easyLogger 输出电压值
    log_i("ADC Voltage: %.3f V", voltage);

    // 5. 释放互斥量
    xSemaphoreGive(xMutex);

    osDelay(100);
  }
}
/* USER CODE END Application */

