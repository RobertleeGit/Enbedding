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
#include "gpio.h"
#include "queue.h"
#include "bsp_key.h"
#include "bsp_led.h"
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
osThreadId_t keyTaskHandle;
const osThreadAttr_t keyTask_attributes = {
  .name = "keyTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

osThreadId_t ledTaskHandle;
const osThreadAttr_t ledTask_attributes = {
  .name = "ledTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

static QueueHandle_t led_key_Queue;

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
void Key_Task(void *argument);
void LED_Task(void *argument);
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
  // 创建队列，长度为 10，单位大小为 led_operation_t
	led_key_Queue = xQueueCreate( 10, sizeof( keyevent_t ) );
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  keyTaskHandle = osThreadNew(Key_Task, NULL, &keyTask_attributes);
  ledTaskHandle = osThreadNew(LED_Task, NULL, &ledTask_attributes);
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
  // uint32_t receive_buffer = 0;

  // // 接受 keyQueue 中的数据
  for(;;)
  {
    // printf("DefaultTask is running\r\n");
    osDelay(100);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */


/**
 * @brief Task function of the Key thread
 * @param[in] argument: the pointer of the function argument 
 * @return None
 */
void Key_Task(void *argument)
{
  static KeyContext_t g_keyContext;       // 按键上下文结构体
  Key_Init(&g_keyContext);
  keyevent_t event = KEY_EVENT_NONE;
  for(;;)
	{
		// printf("KeyThread is running\r\n");
		// 扫描按键获取按键状态
    Key_Scan(&g_keyContext, &event);
    
		if (KEY_EVENT_SHORT_PRESS == g_keyContext.key_event)
    {
      // printf("key event: KEY_EVENT_SHORT_PRESS\r\n");
      if (xQueueSend(led_key_Queue, &g_keyContext.key_event, 0) != pdPASS)
      {
        printf("Failed to post the message\r\n");
      }
    }
    if (KEY_EVENT_LONG_PRESS  == g_keyContext.key_event)
    {
      // printf("key event: KEY_EVENT_LONG_PRESS\r\n");
      if (xQueueSend(led_key_Queue, &g_keyContext.key_event, 0) != pdPASS)
      {
        printf("Failed to post the message\r\n");
      }
    }
		osDelay(10);
	}
}


/**
 * @brief Task function of the LED thread
 * @param[in] argument: the pointer of the function argument 
 * @return None
 */
void LED_Task(void *argument)
{
  keyevent_t event = KEY_EVENT_NONE;   // 事件接受变量

  for(;;)
	{
    // printf("LEDThread is running\r\n");
    // 若队列存在    
    if( led_key_Queue != 0 )
    {
      // 从 keyQueue 中接受数据，若数据还未准备好则阻塞 10 tick 
      if( xQueueReceive(led_key_Queue, &(event), (TickType_t) 10) )
      {
        // 控制 LED
        printf("receive operation successfully\r\n");
        if (KEY_EVENT_SHORT_PRESS == event)
        {
          LED(TOGGLE);
        }
        else if(KEY_EVENT_LONG_PRESS  == event)
        {
          for (size_t i = 0; i < 3; i++)
          {
            LED(ON);
            HAL_Delay(500);
            LED(OFF);
            HAL_Delay(500);
          }
        }
      }
    }
		osDelay(100);
	}
}
/* USER CODE END Application */

