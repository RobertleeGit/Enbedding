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
uint32_t SRAM_function_runtime_record(void);
void SRAM_funtion1(void);
void SRAM_funtion2(void);
uint32_t FLASH_function_runtime_record(void);
void FLASH_funtion1(void);
void FLASH_funtion2(void);
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
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
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
    uint32_t flash_runtime = FLASH_function_runtime_record();
    uint32_t sram_runtime = SRAM_function_runtime_record();

    printf("flash function runtime = %d ms\r\n", flash_runtime);
    printf("sram function runtime = %d ms\r\n", sram_runtime);
    osDelay(100);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
__attribute__((used, section("myram")))
uint32_t SRAM_function_runtime_record(void)
{
  uint32_t start_tick = HAL_GetTick();
  uint32_t end_tick = 0, count = 10000000;
  void (* volatile pfun1) (void) = SRAM_funtion1;
  void (* volatile pfun2) (void) = SRAM_funtion2;

  for (uint32_t i = 0; i < count; i++)
  {
    pfun1();
    pfun2();
  }

  end_tick = HAL_GetTick();
  return end_tick - start_tick;
}

__attribute__((used, section("myram")))
void SRAM_funtion1(void)
{
  int i = 7, j = 8, temp = 0;
  temp = i;
  i = j;
  j = temp;
  return;
}

__attribute__((used, section("myram")))
void SRAM_funtion2(void)
{
  int i = 8, j = 9, temp = 0;
  temp = i;
  i = j;
  j = temp;
  return;
}


uint32_t FLASH_function_runtime_record(void)
{
  uint32_t start_tick = HAL_GetTick();
  uint32_t end_tick = 0, count = 10000000;
  void (* volatile pfun1) (void) = FLASH_funtion1;
  void (* volatile pfun2) (void) = FLASH_funtion2;

  for (uint32_t i = 0; i < count; i++)
  {
    pfun1();
    pfun2();
  }

  end_tick = HAL_GetTick();
  return end_tick - start_tick;
}

void FLASH_funtion1(void)
{
  int i = 7, j = 8, temp = 0;
  temp = i;
  i = j;
  j = temp;
  return;
}

__attribute__((used, section("myflash")))
void FLASH_funtion2(void)
{
  int i = 8, j = 9, temp = 0;
  temp = i;
  i = j;
  j = temp;
  return;
}
/* USER CODE END Application */

