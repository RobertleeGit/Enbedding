/**
  ******************************************************************************
  * @file    bsp_os_freertos.c
  * @brief   OS Bridge — FreeRTOS Concrete Implementation
  *
  *          Implements the ::BSP_OS_Ops_t interface using the FreeRTOS kernel
  *          API.  This is the only file in the application layer that directly
  *          includes FreeRTOS headers.
  *
  *          To switch to a different RTOS:
  *            1. Write a new bsp_os_xxx.c implementing ::BSP_OS_Ops_t.
  *            2. Call BSP_OS_Init() with the new ops table.
  *          No changes to ::led_manager.c or any upper-layer code are required.
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "bsp_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Private variables ---------------------------------------------------------*/

/**
  * @brief  Singleton FreeRTOS operations table — read-only after link.
  */
static const BSP_OS_Ops_t FreeRTOS_Ops;

/* Private function prototypes -----------------------------------------------*/

static BSP_OS_TaskHandle_t OS_TaskCreate(BSP_OS_TaskFunc_t  func,
                                          const char        *name,
                                          uint32_t           stackDepth,
                                          void              *param,
                                          uint32_t           priority);
static void                 OS_TaskDelete(BSP_OS_TaskHandle_t handle);
static void                 OS_DelayMs(uint32_t ms);
static uint32_t             OS_GetTick(void);
static BSP_OS_QueueHandle_t OS_QueueCreate(uint32_t itemSize,
                                            uint32_t queueLen);
static uint8_t              OS_QueueSend(BSP_OS_QueueHandle_t  queue,
                                          const void           *item,
                                          uint32_t              timeoutMs);
static uint8_t              OS_QueueReceive(BSP_OS_QueueHandle_t  queue,
                                             void                 *buffer,
                                             uint32_t              timeoutMs);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Create a FreeRTOS task.
  *
  *          func and param are passed directly through to xTaskCreate;
  *          func's signature `void(*)(void*)` is compatible with
  *          FreeRTOS TaskFunction_t.
  *
  * @param  func       : Task entry function (receives param)
  * @param  name       : Task name (for debug)
  * @param  stackDepth : Stack depth in words
  * @param  param      : Argument passed to func on first invocation
  * @param  priority   : FreeRTOS priority (0 … configMAX_PRIORITIES-1)
  * @retval Task handle or NULL
  */
static BSP_OS_TaskHandle_t OS_TaskCreate(BSP_OS_TaskFunc_t  func,
                                         const char        *name,
                                         uint32_t           stackDepth,
                                         void              *param,
                                         uint32_t           priority)
{
  TaskHandle_t xHandle = NULL;
  BaseType_t   xResult;

  xResult = xTaskCreate((TaskFunction_t)func,
                        (const char *)name,
                        (configSTACK_DEPTH_TYPE)stackDepth,
                        param,
                        (UBaseType_t)priority,
                        &xHandle);

  if (xResult != pdPASS) {
    return NULL;
  }

  return (BSP_OS_TaskHandle_t)xHandle;
}

/**
  * @brief  Delete a FreeRTOS task.
  * @note   handle == NULL → delete the calling task (self-delete).
  * @param  handle : Task handle (NULL = self)
  */
static void OS_TaskDelete(BSP_OS_TaskHandle_t handle)
{
  vTaskDelete((TaskHandle_t)handle);
}

/**
  * @brief  Block for a number of milliseconds.
  * @param  ms : Delay in ms
  */
static void OS_DelayMs(uint32_t ms)
{
  TickType_t ticks = pdMS_TO_TICKS(ms);

  if (ticks == 0U) {
    ticks = 1U;             /* Always delay at least 1 tick */
  }

  vTaskDelay(ticks);
}

/**
  * @brief  Get the FreeRTOS tick count in milliseconds.
  * @note   Assumes configTICK_RATE_HZ == 1000 (1 ms tick).
  * @retval Tick count [ms]
  */
static uint32_t OS_GetTick(void)
{
  return (uint32_t)xTaskGetTickCount();
}

/**
  * @brief  Create a FreeRTOS queue.
  * @param  itemSize : Size of each item in bytes
  * @param  queueLen : Max number of items
  * @retval Queue handle or NULL
  */
static BSP_OS_QueueHandle_t OS_QueueCreate(uint32_t itemSize,
                                            uint32_t queueLen)
{
  QueueHandle_t xQueue;

  xQueue = xQueueCreate((UBaseType_t)queueLen,
                        (UBaseType_t)itemSize);

  return (BSP_OS_QueueHandle_t)xQueue;
}

/**
  * @brief  Send to a FreeRTOS queue.
  * @param  queue     : Queue handle
  * @param  item      : Pointer to data to copy
  * @param  timeoutMs : Max wait time in ms (0 = don't block,
  *                     BSP_OS_WAIT_FOREVER = block forever)
  * @retval 1 on success, 0 on failure
  */
static uint8_t OS_QueueSend(BSP_OS_QueueHandle_t  queue,
                             const void           *item,
                             uint32_t              timeoutMs)
{
  TickType_t ticks;
  BaseType_t result;

  if (timeoutMs == BSP_OS_WAIT_FOREVER) {
    ticks = portMAX_DELAY;
  } else {
    ticks = pdMS_TO_TICKS(timeoutMs);
  }

  result = xQueueSend((QueueHandle_t)queue, item, ticks);

  return (result == pdPASS) ? 1U : 0U;
}

/**
  * @brief  Receive from a FreeRTOS queue (blocking).
  * @param  queue     : Queue handle
  * @param  buffer    : Output buffer
  * @param  timeoutMs : Max wait time in ms
  * @retval 1 on success, 0 on failure
  */
static uint8_t OS_QueueReceive(BSP_OS_QueueHandle_t  queue,
                                void                 *buffer,
                                uint32_t              timeoutMs)
{
  TickType_t ticks;
  BaseType_t result;

  if (timeoutMs == BSP_OS_WAIT_FOREVER) {
    ticks = portMAX_DELAY;
  } else {
    ticks = pdMS_TO_TICKS(timeoutMs);
  }

  result = xQueueReceive((QueueHandle_t)queue, buffer, ticks);

  return (result == pdPASS) ? 1U : 0U;
}

/* Private variables (definition) --------------------------------------------*/

static const BSP_OS_Ops_t FreeRTOS_Ops = {
  .TaskCreate   = OS_TaskCreate,
  .TaskDelete   = OS_TaskDelete,
  .DelayMs      = OS_DelayMs,
  .GetTick      = OS_GetTick,
  .QueueCreate  = OS_QueueCreate,
  .QueueSend    = OS_QueueSend,
  .QueueReceive = OS_QueueReceive,
};

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Obtain the FreeRTOS implementation of the OS bridge.
  * @retval Pointer to the read-only FreeRTOS BSP_OS_Ops_t.
  */
const BSP_OS_Ops_t *BSP_OS_FreeRTOS_GetOps(void)
{
  return &FreeRTOS_Ops;
}
