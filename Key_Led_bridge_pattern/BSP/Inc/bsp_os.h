/**
  ******************************************************************************
  * @file    bsp_os.h
  * @brief   OS Abstraction Layer (Bridge Pattern)
  *
  *          This header defines the OS abstraction interface that decouples
  *          application-layer modules from any specific RTOS implementation.
  *
  *          Architecture:
  *          ┌──────────────────────────┐
  *          │  led_manager / other app │  ⇐ uses BSP_OS_Ops_t only
  *          └────────────┬─────────────┘
  *                       │ BSP_OS_Ops_t vtable (Bridge)
  *          ┌────────────▼─────────────┐
  *          │  bsp_os_freertos.c       │  ⇐ FreeRTOS concrete impl
  *          │  bsp_os_cmsis_rtos.c     │  ⇐ CMSIS-RTOS2 impl (future)
  *          │  bsp_os_threadx.c        │  ⇐ ThreadX impl (future)
  *          └──────────────────────────┘
  *
  *          Porting to a new OS requires only a new concrete implementation
  *          file; no application code changes.
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BSP_OS_H
#define __BSP_OS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/** @addtogroup BSP_OS
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/** @defgroup BSP_OS_Types Exported Types
  * @{
  */

/**
  * @brief  Opaque handle types — concrete definitions live in each backend.
  */
typedef void *BSP_OS_TaskHandle_t;
typedef void *BSP_OS_QueueHandle_t;

/**
  * @brief  Prototype for a task entry function.
  * @param  arg : User-defined argument (cast by the task)
  */
typedef void (*BSP_OS_TaskFunc_t)(void *arg);

/**
  * @brief  OS abstraction operations table (Bridge vtable).
  *
  *          Each concrete OS backend (FreeRTOS, CMSIS-RTOS2, ThreadX, ...)
  *          provides one static instance of this table and registers it
  *          with the LED manager at init time.
  */
typedef struct {
  /* ---- Task Management ---- */

  /**
    * @brief  Create a new task/thread.
    * @param  func       : Task entry function
    * @param  name       : Human-readable task name (debug)
    * @param  stackDepth : Stack size in words
    * @param  param      : Argument passed to func
    * @param  priority   : OS-level priority (backend-specific encoding)
    * @retval Non-NULL task handle on success, NULL on failure
    */
  BSP_OS_TaskHandle_t (*TaskCreate)(BSP_OS_TaskFunc_t  func,
                                     const char        *name,
                                     uint32_t           stackDepth,
                                     void              *param,
                                     uint32_t           priority);

  /**
    * @brief  Delete / terminate a task.
    * @param  handle : Task handle returned by @ref TaskCreate
    */
  void (*TaskDelete)(BSP_OS_TaskHandle_t handle);

  /**
    * @brief  Block the calling task for a specified time.
    * @param  ms : Delay duration in milliseconds
    */
  void (*DelayMs)(uint32_t ms);

  /**
    * @brief  Get a monotonically increasing tick value (milliseconds).
    * @retval Tick count
    */
  uint32_t (*GetTick)(void);

  /* ---- Queue Management ---- */

  /**
    * @brief  Create a fixed-size message queue.
    * @param  itemSize : Size of each queue element [bytes]
    * @param  queueLen : Maximum number of elements in the queue
    * @retval Non-NULL queue handle on success, NULL on failure
    */
  BSP_OS_QueueHandle_t (*QueueCreate)(uint32_t itemSize,
                                       uint32_t queueLen);

  /**
    * @brief  Send an item to the back of a queue.
    * @param  queue     : Queue handle
    * @param  item      : Pointer to the item data
    * @param  timeoutMs : Max wait time (0 = non-blocking,
    *                     BSP_OS_WAIT_FOREVER = block indefinitely)
    * @retval Non-zero on success, 0 on failure/timeout
    */
  uint8_t (*QueueSend)(BSP_OS_QueueHandle_t  queue,
                        const void           *item,
                        uint32_t              timeoutMs);

  /**
    * @brief  Receive an item from the front of a queue (blocking).
    * @param  queue     : Queue handle
    * @param  buffer    : Output buffer (must be at least itemSize)
    * @param  timeoutMs : Max wait time (0 = non-blocking,
    *                     BSP_OS_WAIT_FOREVER = block indefinitely)
    * @retval Non-zero on success, 0 on failure/timeout
    */
  uint8_t (*QueueReceive)(BSP_OS_QueueHandle_t  queue,
                           void                 *buffer,
                           uint32_t              timeoutMs);
} BSP_OS_Ops_t;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup BSP_OS_Constants Exported Constants
  * @{
  */
#define BSP_OS_WAIT_FOREVER      (0xFFFFFFFFU)  /**< Infinite timeout          */
#define BSP_OS_PRIORITY_LOW      (1U)           /**< Low task priority          */
#define BSP_OS_PRIORITY_NORMAL   (2U)           /**< Normal task priority       */
#define BSP_OS_PRIORITY_HIGH     (3U)           /**< High task priority         */
/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/

/** @defgroup BSP_OS_API Exported Functions
  * @{
  */

/**
  * @brief  Obtain the FreeRTOS implementation of the OS bridge.
  * @note   Link with bsp_os_freertos.c.
  * @retval Pointer to the read-only FreeRTOS ::BSP_OS_Ops_t.
  */
const BSP_OS_Ops_t *BSP_OS_FreeRTOS_GetOps(void);

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __BSP_OS_H */
