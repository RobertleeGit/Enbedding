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
#include "iic_hal.h"
#include "bsp_aht21_driver.h"
#include "aht21_iic_bridge.h"
#include <stdio.h>
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
/* Timebase interface: get system tick count in ms */
static uint32_t aht21_timebase_get_tick(void)
{
  return HAL_GetTick();
}

/* Yield interface for RTOS delay */
static uint32_t aht21_rtos_yield(const uint32_t ms)
{
  osDelay(ms);
  return 0;
}
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN Variables */

osThreadId_t defaultTaskHandle;
const osThreadAttr_t aht21Task_attributes = {
    .name = "AHT21Task",
    .stack_size = 256 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};

// IIC bus instance
static iic_bus_t AHT_bus = {
    .IIC_SDA_PORT = GPIOB,
    .IIC_SDA_PIN = GPIO_PIN_13,
    .IIC_SCL_PORT = GPIOB,
    .IIC_SCL_PIN = GPIO_PIN_14,
};

// AHT21 driver instance
static bsp_aht21_driver_t aht21_drv;

static timebase_interface_t aht21_timebase = {
    .pf_get_tick_count = aht21_timebase_get_tick,
};

static yield_interface_t aht21_yield = {
    .pf_rtos_yield = aht21_rtos_yield,
};

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
    .name = "defaultTask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void AHT21_Task(void *argument);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
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

  /* Create AHT21 sensor task */

  osThreadNew(AHT21_Task, NULL, &aht21Task_attributes);

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
  for (;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/**
 * @brief  AHT21 sensor task: initialize and continuously read temp & humidity.
 * @param  argument: Not used
 * @retval None
 */
void AHT21_Task(void *argument)
{
  aht21_status_t ret;
  iic_driver_interface_t *p_iic_interface;

  log_i("AHT21 Task started");

  /* 1: Initialize IIC bridge with bus configuration */
  p_iic_interface = IIC_Drive_Interface_Init(&AHT_bus);
  if (p_iic_interface == NULL)
  {
    log_e("AHT21 IIC bridge init failed");
    osThreadExit();
    return;
  }

  /* 2: Create AHT21 driver instance */
  ret = aht21_create(&aht21_drv, p_iic_interface, &aht21_timebase, &aht21_yield);
  if (AHT21_OK != ret)
  {
    log_e("AHT21 driver create failed: %d", (int)ret);
    osThreadExit();
    return;
  }

  /* 3: Initialize AHT21 sensor */
  ret = aht21_drv.pf_init(&aht21_drv);
  if (AHT21_OK != ret)
  {
    log_e("AHT21 init failed: %d", (int)ret);
    osThreadExit();
    return;
  }

  log_i("AHT21 initialized successfully, starting measurement loop");

  /* 4: Continuous measurement loop */
  for (;;)
  {
    /* Delay 1 second between measurements (per manual recommendation) */
    osDelay(1000);

    float temperature = 0.0f;
    float humidity = 0.0f;

    /* Read temperature */
    ret = aht21_drv.pf_read_temperature(&aht21_drv, &temperature);
    if (AHT21_OK == ret)
    {
      /* Read humidity (separate trigger + read cycle) */
      ret = aht21_drv.pf_read_humidity(&aht21_drv, &humidity);
      if (AHT21_OK == ret)
      {
        printf("AHT21: Temperature = %.2f , Humidity = %.2f %%\r\n",
               temperature, humidity);
      }
      else
      {
        printf("AHT21: Temperature = %.2f, Humidity read failed (%d)\r\n",
               temperature, (int)ret);
      }
    }
    else
    {
      printf("AHT21: Read failed (%d)\r\n", (int)ret);
    }


  }
}

/* USER CODE END Application */
