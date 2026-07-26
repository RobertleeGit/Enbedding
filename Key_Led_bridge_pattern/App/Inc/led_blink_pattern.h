/**
  ******************************************************************************
  * @file    led_blink_pattern.h
  * @brief   LED Blink Pattern — Object-Oriented Blink Controller
  *
  *          This module provides an OOP-style "class" for creating LED blink
  *          pattern objects.  Each object independently manages a single LED's
  *          blinking behavior through an internal state machine.
  *
  *          Key design points:
  *           - LED control goes through ::bsp_led_driver (never direct HAL).
  *           - Time is obtained via an injected ::BSP_Time_Ops_t vtable,
  *             keeping the pattern portable across tick sources.
  *           - Multiple pattern objects can coexist; the manager simply
  *             calls LED_Blink_Tick() on each one periodically.
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
#ifndef __LED_BLINK_PATTERN_H
#define __LED_BLINK_PATTERN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "bsp_led_driver.h"

/** @addtogroup LED_BLINK_PATTERN
  * @{
  */

/* -------------------------------------------------------------------------- */
/*  Time Service Bridge (decouples from HAL / RTOS / bare-metal tick)          */
/* -------------------------------------------------------------------------- */

/** @defgroup LED_BLINK_Time_Bridge Time Abstraction Interface
  * @brief    Injected time operations.  The lower layer supplies concrete
  *           implementations (e.g. HAL_GetTick / osKernelGetTick / DWT cycle
  *           counter).  The blink pattern calls only through this vtable.
  * @{
  */

/**
  * @brief  Get a monotonically increasing tick value in milliseconds.
  * @retval Current tick count [ms]
  */
typedef uint32_t (*BSP_Time_GetTick_t)(void);

/**
  * @brief  Blocking delay in milliseconds (optional — used only if needed).
  * @param  ms : Delay duration in milliseconds
  */
typedef void (*BSP_Time_DelayMs_t)(uint32_t ms);

/**
  * @brief  Time operations table — injected at init time.
  */
typedef struct {
  BSP_Time_GetTick_t  GetTick;   /**< Required: monotonic ms tick              */
  BSP_Time_DelayMs_t  DelayMs;   /**< Optional: blocking delay (may be NULL)   */
} BSP_Time_Ops_t;

/**
  * @}
  */

/* -------------------------------------------------------------------------- */
/*  Blink Pattern "Class"                                                      */
/* -------------------------------------------------------------------------- */

/** @defgroup LED_BLINK_Types Exported Types
  * @{
  */

/**
  * @brief  Blink repetition mode.
  */
typedef enum {
  LED_BLINK_CONTINUOUS = 0x00U,  /**< Blink indefinitely until stopped         */
  LED_BLINK_COUNTED    = 0x01U   /**< Blink exactly ::blinkCount times & stop  */
} LED_Blink_Mode_t;

/**
  * @brief  Internal state-machine phase.
  */
typedef enum {
  LED_BLINK_PHASE_IDLE = 0x00U,  /**< Not running                             */
  LED_BLINK_PHASE_ON   = 0x01U,  /**< LED is lit in the current cycle         */
  LED_BLINK_PHASE_OFF  = 0x02U,  /**< LED is dark in the current cycle        */
  LED_BLINK_PHASE_DONE = 0x03U   /**< Counted blinks completed                */
} LED_Blink_Phase_t;

/**
  * @brief  Blink pattern status codes.
  */
typedef enum {
  LED_BLINK_OK       = 0x00U,    /**< Operation successful                     */
  LED_BLINK_ERROR    = 0x01U,    /**< Parameter / state error                  */
  LED_BLINK_ACTIVE   = 0x02U,    /**< Pattern is currently running             */
  LED_BLINK_INACTIVE = 0x03U     /**< Pattern is stopped / done                */
} LED_Blink_Status_t;

/**
  * @brief  Immutable configuration for one blink-pattern object.
  *
  *          Set once at creation, may be updated at runtime via
  *          ::LED_Blink_SetConfig.
  */
typedef struct {
  uint32_t cycleMs;              /**< Full blink cycle period [ms]              */
  uint8_t  onRatio;              /**< Duty ratio: ON percentage [0-100]        */
  uint32_t blinkCount;           /**< Number of blinks (0 = continuous)        */
  LED_Blink_Mode_t mode;         /**< ::LED_BLINK_CONTINUOUS or ::LED_BLINK_COUNTED */
} LED_Blink_Config_t;

/**
  * @brief  Blink pattern handle ("object instance").
  *
  *          Allocated by the caller (static or dynamic).  The caller is
  *          responsible for lifetime management.
  */
typedef struct {
  /* ---- Dependencies (injected at init) ---- */
  BSP_LED_HandleTypeDef  *hled;      /**< Bound LED handle (driver layer)      */
  const BSP_Time_Ops_t   *pTimeOps;  /**< Time service vtable                  */

  /* ---- Configuration ---- */
  LED_Blink_Config_t      config;    /**< Active configuration                 */

  /* ---- State machine ---- */
  LED_Blink_Phase_t       phase;     /**< Current phase                        */
  uint32_t                phaseTick; /**< Tick value at phase entry            */
  uint32_t                blinkIdx;  /**< Completed blink counter              */
  uint8_t                 running;   /**< 1 = state machine is active          */
} LED_Blink_Pattern_t;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup LED_BLINK_Constants Exported Constants
  * @{
  */
#define LED_BLINK_ON_RATIO_MIN      (0U)    /**< 0 % — always off              */
#define LED_BLINK_ON_RATIO_MAX      (100U)  /**< 100 % — always on             */
#define LED_BLINK_COUNT_INFINITE    (0U)    /**< Sentinel for continuous mode  */
/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/

/** @defgroup LED_BLINK_API Public API
  * @brief    Construct, start, stop, and tick the blink state machine.
  *
  *          Typical usage:
  *          @code
  *          // 1. Allocate
  *          LED_Blink_Pattern_t blinkObj;
  *
  *          // 2. Configure
  *          LED_Blink_Config_t cfg = {
  *            .cycleMs = 500, .onRatio = 50,
  *            .blinkCount = 3, .mode = LED_BLINK_COUNTED
  *          };
  *
  *          // 3. Init (injects LED handle + time ops)
  *          LED_Blink_Init(&blinkObj, &hled, &g_TimeOps, &cfg);
  *
  *          // 4. Start
  *          LED_Blink_Start(&blinkObj);
  *
  *          // 5. Tick periodically (e.g. every 1 ms from a timer ISR or task)
  *          void Timer_Callback(void) { LED_Blink_Tick(&blinkObj); }
  *          @endcode
  * @{
  */

/**
  * @brief  Construct / initialise a blink pattern object.
  * @note   The LED must already be initialised via ::BSP_LED_Init.
  *          This function only stores references; it does not copy the config.
  * @param  pObj     : Pointer to caller-allocated pattern object
  * @param  hled     : Pointer to an initialised LED driver handle
  * @param  pTimeOps : Pointer to time operations vtable
  * @param  pConfig  : Pointer to blink configuration
  * @retval LED_Blink_Status_t
  */
LED_Blink_Status_t LED_Blink_Init(LED_Blink_Pattern_t      *pObj,
                                   BSP_LED_HandleTypeDef    *hled,
                                   const BSP_Time_Ops_t     *pTimeOps,
                                   const LED_Blink_Config_t *pConfig);

/**
  * @brief  Reset the pattern object to its initial (stopped) state.
  * @param  pObj : Pointer to pattern object
  * @retval LED_Blink_Status_t
  */
LED_Blink_Status_t LED_Blink_Reset(LED_Blink_Pattern_t *pObj);

/**
  * @brief  Start (or restart) the blink pattern.
  * @note   If already running the counters are reset.
  * @param  pObj : Pointer to pattern object
  * @retval LED_Blink_Status_t
  */
LED_Blink_Status_t LED_Blink_Start(LED_Blink_Pattern_t *pObj);

/**
  * @brief  Stop the blink pattern immediately.
  * @note   The LED is turned off.
  * @param  pObj : Pointer to pattern object
  * @retval LED_Blink_Status_t
  */
LED_Blink_Status_t LED_Blink_Stop(LED_Blink_Pattern_t *pObj);

/**
  * @brief  State-machine tick — must be called periodically.
  *
  *          The caller (manager / timer ISR / RTOS task) invokes this
  *          function at a regular interval.  It compares the elapsed time
  *          against the configured phase durations and transitions the
  *          state machine as needed.
  *
  * @note   Calling frequency should be significantly higher than the blink
  *         frequency (e.g. 1 ms tick for a 100 ms cycle).
  * @param  pObj : Pointer to pattern object
  * @retval LED_Blink_Status_t
  */
LED_Blink_Status_t LED_Blink_Tick(LED_Blink_Pattern_t *pObj);

/**
  * @brief  Change the configuration at runtime.
  * @note   Takes effect at the next phase boundary.
  * @param  pObj    : Pointer to pattern object
  * @param  pConfig : Pointer to new configuration
  * @retval LED_Blink_Status_t
  */
LED_Blink_Status_t LED_Blink_SetConfig(LED_Blink_Pattern_t      *pObj,
                                        const LED_Blink_Config_t *pConfig);

/**
  * @brief  Query whether the pattern is currently active.
  * @param  pObj : Pointer to pattern object
  * @retval 0 = inactive (stopped / done), 1 = active
  */
uint8_t LED_Blink_IsActive(const LED_Blink_Pattern_t *pObj);

/**
  * @brief  Query the current blink index (0 … blinkCount-1).
  * @param  pObj : Pointer to pattern object
  * @retval Current blink counter
  */
uint32_t LED_Blink_GetCount(const LED_Blink_Pattern_t *pObj);

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __LED_BLINK_PATTERN_H */
