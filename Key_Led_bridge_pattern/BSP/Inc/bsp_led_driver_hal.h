/**
  ******************************************************************************
  * @file    bsp_led_driver_hal.h
  * @brief   LED Driver — HAL Concrete Implementation Header
  *
  *          Provides the HAL-specific configuration structure and a convenience
  *          one-shot initialization function.  Include this header only in
  *          modules that need to configure LEDs with the HAL backend.
  *
  *          Usage example:
  *          @code
  *          BSP_LED_HandleTypeDef hled;
  *          BSP_LED_HAL_Data_t    halData;
  *          BSP_LED_HAL_Config_t  cfg = {
  *            .port = LED_GPIO_Port, .pin = LED_Pin,
  *            .activeLevel = BSP_LED_ACTIVE_LOW,
  *            .htim = NULL, .timChannel = 0
  *          };
  *          BSP_LED_HAL_Init(&hled, &halData, &cfg);
  *          BSP_LED_On(&hled);
  *          @endcode
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
#ifndef __BSP_LED_DRIVER_HAL_H
#define __BSP_LED_DRIVER_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "bsp_led_driver.h"
#include "stm32f4xx_hal.h"

/** @addtogroup BSP_LED_DRIVER_HAL
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/** @defgroup BSP_LED_HAL_Exported_Types Exported Types
  * @{
  */

/**
  * @brief  HAL implementation configuration structure.
  *
  *          Passed to ::BSP_LED_Init (via ::BSP_LED_HAL_Init) to describe the
  *          physical LED hardware connections and optional PWM timer.
  */
typedef struct {
  GPIO_TypeDef       *port;         /**< GPIO port for the LED                  */
  uint16_t            pin;          /**< GPIO pin for the LED                   */
  uint8_t             activeLevel;  /**< @ref BSP_LED_ACTIVE_HIGH or
                                         @ref BSP_LED_ACTIVE_LOW               */
  TIM_HandleTypeDef  *htim;         /**< Timer handle for PWM (NULL if unused)  */
  uint32_t            timChannel;   /**< Timer PWM channel (e.g. TIM_CHANNEL_1) */
} BSP_LED_HAL_Config_t;

/**
  * @brief  HAL-specific per-LED internal data.
  *
  *          The user statically allocates one instance per LED and passes it
  *          to ::BSP_LED_HAL_Init.  The contents are managed by the HAL backend.
  */
typedef struct {
  GPIO_TypeDef       *port;         /**< GPIO port                       */
  uint16_t            pin;          /**< GPIO pin                        */
  uint8_t             activeLevel;  /**< Active-high or active-low       */
  TIM_HandleTypeDef  *htim;         /**< Timer handle for PWM            */
  uint32_t            timChannel;   /**< Timer channel for PWM           */
} BSP_LED_HAL_Data_t;

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/

/** @defgroup BSP_LED_HAL_Exported_Functions Exported Functions
  * @{
  */

/**
  * @brief  One-shot HAL initialization: registers the HAL implementation,
  *         assigns the internal data buffer, and initializes the LED hardware.
  *
  *          This is the recommended entry point for HAL-based projects.
  *          It performs the following steps:
  *           1. Zero-initializes the handle and HAL data.
  *           2. Links the HAL data to the handle.
  *           3. Registers the HAL operations table.
  *           4. Calls ::BSP_LED_Init.
  *
  * @param  hled    : Pointer to LED handle (user-allocated)
  * @param  pData   : Pointer to HAL data buffer (user-allocated)
  * @param  pConfig : Pointer to HAL configuration
  * @retval BSP_LED_Status_t
  */
BSP_LED_Status_t BSP_LED_HAL_Init(BSP_LED_HandleTypeDef *hled,
                                   BSP_LED_HAL_Data_t    *pData,
                                   const BSP_LED_HAL_Config_t *pConfig);

/**
  * @brief  Obtain the HAL implementation operations table.
  * @note   Normally called internally by ::BSP_LED_HAL_Init; exposed for
  *         advanced use cases that require manual registration.
  * @retval Pointer to the read-only HAL operations table.
  */
const BSP_LED_ImplOps_t *BSP_LED_HAL_GetOps(void);

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __BSP_LED_DRIVER_HAL_H */
