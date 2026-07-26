/**
  ******************************************************************************
  * @file    led_manager.h
  * @brief   LED Manager — Message-Queue-Driven LED Object Manager
  *
  *          This module is the top-level orchestrator for all LED pattern
  *          objects.  It provides a non-blocking, message-queue-based API
  *          that upper-layer application code uses to control LEDs.
  *
  *          Architecture:
  *          ┌───────────────────────────────────┐
  *          │  Application / main / freertos.c  │ sends ::LED_Mgr_SendCmd()
  *          └──────────────┬────────────────────┘
  *                         │ msg queue
  *          ┌──────────────▼────────────────────┐
  *          │  led_manager                      │
  *          │   ┌───────────────────────────┐   │
  *          │   │ LED_Mgr_Task (loops)      │   │  dequeue cmd → dispatch
  *          │   └───────────────────────────┘   │
  *          │   ┌───────────────────────────┐   │
  *          │   │ Per-LED control tasks     │   │  Tick LED_x_pattern
  *          │   │ (created on START,        │   │
  *          │   │  deleted on STOP / DONE)  │   │
  *          │   └───────────────────────────┘   │
  *          └────────────┬──────────────────────┘
  *                       │ BSP_OS_Ops_t (Bridge)
  *          ┌────────────▼──────────────────────┐
  *          │  bsp_os_freertos.c                │
  *          └───────────────────────────────────┘
  *
  *          Key design points:
  *           - **Non-blocking**: the caller sends a command via a queue
  *             and returns immediately; the manager processes it in its own
  *             task context.
  *           - **OS-decoupled**: all OS interactions go through ::BSP_OS_Ops_t.
  *           - **Extensible**: ::LED_Mgr_Mode_t enumerates supported modes;
  *             adding a new pattern type (e.g. fade) requires only adding a
  *             new union member and a new ::LED_Mgr_CmdId_t.
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
#ifndef __LED_MANAGER_H
#define __LED_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "bsp_led_driver.h"
#include "led_blink_pattern.h"
#include "bsp_os.h"

/** @addtogroup LED_MANAGER
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/** @defgroup LED_MGR_Types Exported Types
  * @{
  */

/**
  * @brief  LED operating mode enumeration.
  *
  *          Each LED slot can be in exactly one mode at a time.
  *          Extend this enum when adding new pattern types.
  */
typedef enum {
  LED_MGR_MODE_OFF   = 0x00U,   /**< LED is idle / not controlled            */
  LED_MGR_MODE_ON    = 0x01U,   /**< LED forced on (solid)                   */
  LED_MGR_MODE_BLINK = 0x02U,   /**< LED running ::LED_Blink_Pattern_t       */
  LED_MGR_MODE_COUNT           /**< Sentinel — must be last                  */
} LED_Mgr_Mode_t;

/**
  * @brief  Command IDs recognised by the manager.
  *
  *          Upper layers send these via ::LED_Mgr_SendCmd.  The manager task
  *          dequeues and dispatches.
  */
typedef enum {
  LED_MGR_CMD_START_BLINK = 0x00U,  /**< Start (or restart) blink pattern     */
  LED_MGR_CMD_STOP        = 0x01U,  /**< Stop LED & free resources            */
  LED_MGR_CMD_SET_CONFIG  = 0x02U,  /**< Update blink config at runtime        */
} LED_Mgr_CmdId_t;

/**
  * @brief  Command packet sent through the message queue.
  */
typedef struct {
  LED_Mgr_CmdId_t cmdId;          /**< What to do                             */
  uint8_t         ledId;          /**< Target LED (0 … LED_MGR_MAX_LEDS-1)    */
  BSP_OS_QueueHandle_t notifyQueue; /**< Optional: queue for completion notify  */

  union {
    LED_Blink_Config_t blinkCfg;  /**< Valid for START_BLINK / SET_CONFIG     */
  } data;                         /**< Per-command payload                    */
} LED_Mgr_Cmd_t;

/**
  * @brief  Per-LED registration entry held by the manager.
  *
  *          Each slot holds the full state of one managed LED:
  *          its driver handle, current mode, pattern object, and
  *          the OS task that runs the pattern tick loop.
  */
typedef struct {
  uint8_t                id;            /**< LED identifier                    */
  LED_Mgr_Mode_t         mode;          /**< Current operating mode            */
  BSP_LED_HandleTypeDef *hled;          /**< Bound LED driver handle           */
  BSP_OS_TaskHandle_t    taskHandle;    /**< Control task (NULL if idle)       */
  const BSP_Time_Ops_t  *pTimeOps;      /**< Time service for pattern objects  */
  BSP_OS_QueueHandle_t   notifyQueue;   /**< Completion notification target    */

  /**
    * @brief  Union of pattern objects — one per supported mode.
    */
  union {
    LED_Blink_Pattern_t blink;          /**< ::LED_MGR_MODE_BLINK              */
  } pattern;                            /**< Expand when adding new modes       */
} LED_Mgr_LedObj_t;

/**
  * @brief  Manager initialisation structure.
  */
typedef struct {
  const BSP_OS_Ops_t  *pOsOps;          /**< OS bridge vtable                  */
  const BSP_Time_Ops_t *pTimeOps;       /**< Time service vtable               */
  LED_Mgr_LedObj_t     *pLedPool;       /**< Pre-allocated LED object array    */
  uint8_t               ledPoolSize;    /**< Number of slots in the pool       */
  uint32_t              cmdQueueLen;    /**< Command queue depth (> 0)         */
  uint32_t              ctrlTaskStack;  /**< Stack depth for each control task */
  uint32_t              ctrlTaskPrio;   /**< Priority for each control task    */
} LED_Mgr_Init_t;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup LED_MGR_Constants Exported Constants
  * @{
  */
#define LED_MGR_MAX_LEDS          (8U)    /**< Maximum managed LED count       */
#define LED_MGR_CMD_QUEUE_LEN     (16U)   /**< Default command queue depth     */
#define LED_MGR_CTRL_TASK_STACK   (128U)  /**< Default control-task stack words*/
#define LED_MGR_CTRL_TASK_PRIO    (BSP_OS_PRIORITY_NORMAL) /**< Ctrl task prio */
/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/

/** @defgroup LED_MGR_API Public API
  *
  *          Typical usage:
  *          @code
  *          // 1. Prepare LED objects
  *          LED_Mgr_LedObj_t pool[2];
  *          uint8_t poolSize = sizeof(pool) / sizeof(pool[0]);
  *
  *          // 2. Register LEDs into the pool
  *          LED_Mgr_RegisterLed(pool, poolSize, 0, &hled0);
  *          LED_Mgr_RegisterLed(pool, poolSize, 1, &hled1);
  *
  *          // 3. Init manager (creates queue & main task)
  *          LED_Mgr_Init_t init = {
  *            .pOsOps    = BSP_OS_FreeRTOS_GetOps(),
  *            .pTimeOps  = &g_TimeOps,
  *            .pLedPool  = pool,
  *            .ledPoolSize = poolSize,
  *            .cmdQueueLen = LED_MGR_CMD_QUEUE_LEN,
  *            .ctrlTaskStack = LED_MGR_CTRL_TASK_STACK,
  *            .ctrlTaskPrio  = LED_MGR_CTRL_TASK_PRIO,
  *          };
  *          LED_Mgr_Init(&init);
  *
  *          // 4. From application (non-blocking):
  *          LED_Mgr_Cmd_t cmd = {
  *            .cmdId = LED_MGR_CMD_START_BLINK,
  *            .ledId = 0,
  *            .data.blinkCfg = { .cycleMs=500, .onRatio=30,
  *                               .blinkCount=5, .mode=LED_BLINK_COUNTED }
  *          };
  *          LED_Mgr_SendCmd(&cmd);
  *          @endcode
  * @{
  */

/**
  * @brief  Register an LED handle with the manager's object pool.
  * @note   Must be called once per LED, before ::LED_Mgr_Init.
  * @param  pPool    : Pointer to the pre-allocated LED object array
  * @param  poolSize : Number of slots in the pool
  * @param  ledId    : Logical LED ID (0 … poolSize - 1)
  * @param  hled     : Pointer to an initialised LED driver handle
  * @retval 0 on success, non-zero on failure
  */
uint8_t LED_Mgr_RegisterLed(LED_Mgr_LedObj_t     *pPool,
                            uint8_t               poolSize,
                            uint8_t               ledId,
                            BSP_LED_HandleTypeDef *hled);

/**
  * @brief  Initialise the LED manager.
  *
  *          Creates the command queue and the main manager task.
  *          After this call the manager is ready to process commands.
  *
  * @param  pInit : Pointer to manager configuration
  * @retval 0 on success, non-zero on failure
  */
uint8_t LED_Mgr_Init(const LED_Mgr_Init_t *pInit);

/**
  * @brief  Send a control command to the manager (non-blocking).
  *
  *          The command is copied into the message queue; this function
  *          returns immediately.  The manager task processes it
  *          asynchronously.
  *
  * @param  pCmd : Pointer to ::LED_Mgr_Cmd_t
  * @retval 0 on success, non-zero on failure (queue full / uninitialised)
  */
uint8_t LED_Mgr_SendCmd(const LED_Mgr_Cmd_t *pCmd);

/**
  * @brief  Stop all LEDs and shut down the manager.
  * @retval 0 on success
  */
uint8_t LED_Mgr_DeInit(void);

/**
  * @brief  Query a LED's current operating mode.
  * @param  ledId : LED ID
  * @retval ::LED_Mgr_Mode_t (::LED_MGR_MODE_OFF if invalid ID)
  */
LED_Mgr_Mode_t LED_Mgr_GetMode(uint8_t ledId);

/**
  * @brief  Query whether a LED is currently active.
  * @param  ledId : LED ID
  * @retval 1 = active, 0 = inactive / invalid
  */
uint8_t LED_Mgr_IsActive(uint8_t ledId);

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __LED_MANAGER_H */
