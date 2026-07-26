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
#include "led_manager.h"
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
extern BSP_LED_HandleTypeDef hled;             // LED 操作句柄（main.c 中定义）
LED_Mgr_LedObj_t ledPool[1];                   // 管理者维护的 LED 对象池（目前 1 个 LED）
static BSP_OS_QueueHandle_t gNotifyQueue = NULL;  // 闪烁完成通知队列
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
static uint32_t MyGetTick(void) { return HAL_GetTick(); }

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
  static const BSP_Time_Ops_t timeOps = { .GetTick = MyGetTick, .DelayMs = NULL };

  /* Register LED 0 with the manager's object pool */
  (void)LED_Mgr_RegisterLed(ledPool,
                             sizeof(ledPool) / sizeof(ledPool[0]),
                             0U,
                             &hled);

  /* Create notification queue for completion callbacks (depth 4 × uint8_t) */
  gNotifyQueue = BSP_OS_FreeRTOS_GetOps()->QueueCreate(sizeof(uint8_t), 4U);

  /* Initialise the LED manager (creates queue + manager task) */
  LED_Mgr_Init_t mgrInit = {
    .pOsOps        = BSP_OS_FreeRTOS_GetOps(),
    .pTimeOps      = &timeOps,
    .pLedPool      = ledPool,
    .ledPoolSize   = sizeof(ledPool) / sizeof(ledPool[0]),
    .cmdQueueLen   = LED_MGR_CMD_QUEUE_LEN,
    .ctrlTaskStack = LED_MGR_CTRL_TASK_STACK,
    .ctrlTaskPrio  = LED_MGR_CTRL_TASK_PRIO,
  };
  LED_Mgr_Init(&mgrInit);
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
  /* Avoid unused-argument warning */
  (void)argument;

  for (;;)
  {
    /*
     * Step 1: Send a START_BLINK command.
     *         500 ms cycle, 1:1 (50 %) duty, 5 counted blinks.
     *         notifyQueue = gNotifyQueue → manager sends ledId back on finish.
     */
    LED_Mgr_Cmd_t cmd = {
      .cmdId       = LED_MGR_CMD_START_BLINK,
      .ledId       = 0U,
      .notifyQueue = gNotifyQueue,
      .data.blinkCfg = {
        .cycleMs    = 500U,                /* 500 ms period                   */
        .onRatio    = 50U,                 /* 50 % duty (1:1)                 */
        .blinkCount = 5U,                  /* 5 blinks then stop              */
        .mode       = LED_BLINK_COUNTED,
      },
    };
    (void)LED_Mgr_SendCmd(&cmd);

    /*
     * Step 2: Block until the manager notifies completion.
     *         The manager's control task sends the ledId via gNotifyQueue
     *         when the counted-blink pattern finishes.
     */
    uint8_t finishedLedId;
    (void)BSP_OS_FreeRTOS_GetOps()->QueueReceive(gNotifyQueue, &finishedLedId, BSP_OS_WAIT_FOREVER);

    /*
     * Step 3: Wait 1 s, then loop back to Step 1.
     */
    osDelay(1000U);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

