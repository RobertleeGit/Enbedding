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
#include "queue.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "iic_hal.h"
#include "bsp_aht21_driver.h"
#include "bsp_aht21_handler.h"
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

osThreadId_t aht21TaskHandle;
const osThreadAttr_t aht21Task_attributes = {
    .name = "AHT21Task",
    .stack_size = 256 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};

osThreadId_t aht21SenderTaskHandle;
const osThreadAttr_t aht21SenderTask_attributes = {
    .name = "AHT21Sender",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};

// IIC bus instance to create IIC driver instance
static iic_bus_t AHT_bus = {
    .IIC_SDA_PORT = GPIOB,
    .IIC_SDA_PIN = GPIO_PIN_13,
    .IIC_SCL_PORT = GPIOB,
    .IIC_SCL_PIN = GPIO_PIN_14,
};

// IIC driver interface instance
static iic_driver_interface_t aht21_iic_interface;

// Timebase instance
static timebase_interface_t aht21_timebase = {
    .pf_get_tick_count = aht21_timebase_get_tick,
};

// OS yield instance
static yield_interface_t aht21_yield = {
    .pf_rtos_yield = aht21_rtos_yield,
};

// --- Forward declarations for OS interface implementations ---
static aht21_handler_status_t aht21_freertos_queue_create(void ** const queue_handle,
                                                           uint32_t queue_length,
                                                           uint32_t item_size);
static aht21_handler_status_t aht21_freertos_queue_send(void ** const queue_handle,
                                                         void * const item,
                                                         uint32_t timeout);
static aht21_handler_status_t aht21_freertos_queue_receive(void ** const queue_handle,
                                                            void * const item,
                                                            uint32_t timeout);
static void aht21_os_delay(uint32_t ms);

// OS interface instance
static os_interface_t aht21_os_interface = {
    .os_delay_ms      = aht21_os_delay,
    .os_queue_create  = aht21_freertos_queue_create,
    .os_queue_send    = aht21_freertos_queue_send,
    .os_queue_receive = aht21_freertos_queue_receive,
};

// Handler function table — input resources passed to AHT21_Handler_Task
static handler_function_table_t aht21_func_table = {
    .p_os_interface        = &aht21_os_interface,
    .p_iic_driver_interface = &aht21_iic_interface,
    .p_timebase_interface  = &aht21_timebase,
    .p_yield_interface     = &aht21_yield,
};

// Client handle — filled by the handler task; used by external threads to
// send events into the handler's event queue.
static aht21_handler_client_t aht21_client = {
    .event_queue_handle    = NULL,
};

// Task initialisation — bundles input resources and output client for the
// handler task.  Passed as the single void* argument to AHT21_Handler_Task.
static aht21_task_init_t aht21_task_init = {
    .p_func_table = &aht21_func_table,
    .p_client     = &aht21_client,
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

static void AHT21_Sender_Task(void *argument);

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

  /* Initialise the IIC bridge once before any task uses it */
  IIC_Drive_Interface_Init(&aht21_iic_interface, &AHT_bus);

  /* Create the AHT21 handler task — passes the task-init struct bundling
     the function table (input) and client handle (output).                 */
  aht21TaskHandle = osThreadNew(AHT21_Handler_Task, &aht21_task_init, &aht21Task_attributes);

  /* Create the sender task �?periodically posts read requests to the handler */
  aht21SenderTaskHandle = osThreadNew(AHT21_Sender_Task, NULL, &aht21SenderTask_attributes);

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

/*==============================================================================
 * OS Interface Implementations
 *   Wraps FreeRTOS CMSIS-V2 APIs to match the os_interface_t function table.
 *============================================================================*/

/**
 * @brief  Create a message queue (maps to osMessageQueueNew).
 */
static aht21_handler_status_t aht21_freertos_queue_create(void ** const queue_handle,
                                                           uint32_t queue_length,
                                                           uint32_t item_size)
{
    osMessageQueueId_t mq;

    mq = osMessageQueueNew(queue_length, item_size, NULL);
    if (mq == NULL)
    {
        return AHT21_HANDLER_ERROR_RESOURCE;
    }

    *queue_handle = (void *)mq;
    return AHT21_HANDLER_OK;
}

/**
 * @brief  Send an item to a message queue (maps to osMessageQueuePut).
 */
static aht21_handler_status_t aht21_freertos_queue_send(void ** const queue_handle,
                                                         void * const item,
                                                         uint32_t timeout)
{
    osStatus_t status;
    osMessageQueueId_t mq;

    mq     = (osMessageQueueId_t)(*queue_handle);
    status = osMessageQueuePut(mq, item, 0U, timeout);

    if (status != osOK)
    {
        return (status == osErrorTimeout) ? AHT21_HANDLER_ERROR_TIMEOUT
                                          : AHT21_HANDLER_ERROR;
    }
    return AHT21_HANDLER_OK;
}

/**
 * @brief  Receive an item from a message queue (maps to osMessageQueueGet).
 */
static aht21_handler_status_t aht21_freertos_queue_receive(void ** const queue_handle,
                                                            void * const item,
                                                            uint32_t timeout)
{
    osStatus_t status;
    osMessageQueueId_t mq;

    mq     = (osMessageQueueId_t)(*queue_handle);
    status = osMessageQueueGet(mq, item, NULL, timeout);

    if (status != osOK)
    {
        return (status == osErrorTimeout) ? AHT21_HANDLER_ERROR_TIMEOUT
                                          : AHT21_HANDLER_ERROR;
    }
    return AHT21_HANDLER_OK;
}

/**
 * @brief  Blocking OS delay (maps to osDelay).
 */
static void aht21_os_delay(uint32_t ms)
{
    osDelay(ms);
}


/*==============================================================================
 * Callback invoked by the handler task after each successful read.
 *============================================================================*/

/**
 * @brief  Print the temperature and humidity via printf.
 * @param  temperature: Pointer to the temperature value
 * @param  humidity:    Pointer to the humidity value
 */
static void aht21_printf_callback(float *temperature, float *humidity)
{
    if ((temperature != NULL) && (humidity != NULL))
    {
        printf("AHT21: Temperature = %.2f C, Humidity = %.2f %%\r\n", *temperature, *humidity);
    }
}


/*==============================================================================
 * Sender Task periodically posts read-requests to the handler's event queue.
 *============================================================================*/

/**
 * @brief  Sender task: waits for the handler to initialise, then periodically
 *         sends BOTH-type read events to the handler's event queue.
 * @param  argument: Not used
 * @retval None
 */
static void AHT21_Sender_Task(void *argument)
{
    float temp_buf;
    float humi_buf;
    aht21_handler_event_t event;
    aht21_handler_status_t ret;

    (void)argument;   /* unused */

    log_i("AHT21 Sender Task started");

    /* Wait until the handler task has created the queue and exposed its handle */
    while (aht21_client.event_queue_handle == NULL)
    {
        osDelay(50);
    }

    log_i("AHT21 Sender: handler ready, starting periodic reads");

    /* Prepare the event template reused every cycle */
    event.temperature  = &temp_buf;
    event.humidity     = &humi_buf;
    event.type         = BOTH;
    event.lifetime     = 1000;           /* 1000 ms cache window                  */
    event.pf_callback  = aht21_printf_callback;

    for (;;)
    {
        /* Measurement period: 2 seconds */
        osDelay(2000);

        ret = aht21_func_table.p_os_interface->os_queue_send(
                    &aht21_client.event_queue_handle,
                    (void *)&event,
                    100);

        if (AHT21_HANDLER_OK != ret)
        {
            log_w("AHT21 Sender: queue send failed, ret: %d", ret);
        }
    }
}

/* USER CODE END Application */
