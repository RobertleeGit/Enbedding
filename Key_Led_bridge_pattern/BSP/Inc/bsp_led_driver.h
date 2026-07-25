/**
  ******************************************************************************
  * @file    bsp_led_driver.h
  * @brief   LED Driver Abstraction Layer — Public API & Bridge Interface
  *
  *          This module implements the <b>Bridge Pattern</b> to decouple the
  *          high-level LED driver API from the low-level hardware implementation.
  *
  *          +-------------------+         +-------------------------+
  *          |  bsp_led_driver   |  bridge |  Concrete Implementation |
  *          |  (Abstraction)    |-------->|  (e.g. HAL, LL, custom) |
  *          +-------------------+         +-------------------------+
  *
  *          To port to a different hardware library or platform, supply a new
  *          ::BSP_LED_ImplOps_t implementation — the driver API remains unchanged.
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
#ifndef __BSP_LED_DRIVER_H
#define __BSP_LED_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/** @addtogroup BSP_LED_DRIVER
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/** @defgroup BSP_LED_Exported_Types Exported Types
  * @{
  */

/**
  * @brief  LED logical state enumeration.
  */
typedef enum {
  BSP_LED_STATE_OFF = 0x00U,   /**< LED is off            */
  BSP_LED_STATE_ON  = 0x01U    /**< LED is on             */
} BSP_LED_State_t;

/**
  * @brief  LED driver operation status codes.
  */
typedef enum {
  BSP_LED_OK       = 0x00U,    /**< Operation successful   */
  BSP_LED_ERROR    = 0x01U,    /**< General error          */
  BSP_LED_BUSY     = 0x02U,    /**< Resource busy          */
  BSP_LED_TIMEOUT  = 0x03U     /**< Operation timed out    */
} BSP_LED_Status_t;

/**
  * @brief  LED electrical active level definition.
  */
typedef enum {
  BSP_LED_ACTIVE_HIGH = 0x00U, /**< Logic 1 turns LED on   */
  BSP_LED_ACTIVE_LOW  = 0x01U  /**< Logic 0 turns LED on   */
} BSP_LED_ActiveLevel_t;

/**
  * @brief  Forward declaration of the implementation operations table.
  */
typedef struct BSP_LED_ImplOps_s BSP_LED_ImplOps_t;

/**
  * @brief  LED handle structure.
  *
  *          Holds the bridge to a concrete implementation plus runtime state.
  *          The user must set @ref pOps and @ref pImplData before calling
  *          any API function (use the convenience init from the implementation).
  */
typedef struct {
  const BSP_LED_ImplOps_t *pOps;      /**< Bridge: pointer to implementation ops   */
  void                    *pImplData; /**< Opaque pointer to implementation data   */
  BSP_LED_State_t          state;     /**< Logical LED state                       */
  uint8_t                  brightness;/**< Current brightness level (0 - 255)      */
} BSP_LED_HandleTypeDef;

/**
  * @brief  LED hardware abstraction operations interface (Bridge Implementation).
  *
  *          Each function pointer represents a hardware operation that must be
  *          provided by a concrete backend.  New backends (HAL, LL, StdPeriph,
  *          or a different MCU altogether) implement this table and register it
  *          with the handle.
  */
struct BSP_LED_ImplOps_s {
  /**
    * @brief  Initialize LED hardware.
    * @param  hled    : Pointer to LED handle
    * @param  pConfig : Pointer to implementation-specific configuration
    * @retval BSP_LED_Status_t
    */
  BSP_LED_Status_t (*Init)(BSP_LED_HandleTypeDef *hled, const void *pConfig);

  /**
    * @brief  De-initialize LED hardware.
    * @param  hled : Pointer to LED handle
    * @retval BSP_LED_Status_t
    */
  BSP_LED_Status_t (*DeInit)(BSP_LED_HandleTypeDef *hled);

  /**
    * @brief  Turn LED on (hardware level).
    * @param  hled : Pointer to LED handle
    * @retval BSP_LED_Status_t
    */
  BSP_LED_Status_t (*On)(BSP_LED_HandleTypeDef *hled);

  /**
    * @brief  Turn LED off (hardware level).
    * @param  hled : Pointer to LED handle
    * @retval BSP_LED_Status_t
    */
  BSP_LED_Status_t (*Off)(BSP_LED_HandleTypeDef *hled);

  /**
    * @brief  Toggle LED output (hardware level).
    * @param  hled : Pointer to LED handle
    * @retval BSP_LED_Status_t
    */
  BSP_LED_Status_t (*Toggle)(BSP_LED_HandleTypeDef *hled);

  /**
    * @brief  Set LED brightness via PWM (hardware level).
    * @param  hled       : Pointer to LED handle
    * @param  brightness : Brightness value [0 = off, 255 = full on]
    * @retval BSP_LED_Status_t
    */
  BSP_LED_Status_t (*SetBrightness)(BSP_LED_HandleTypeDef *hled,
                                     uint8_t brightness);
};

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup BSP_LED_Exported_Constants Exported Constants
  * @{
  */
#define BSP_LED_BRIGHTNESS_MIN       (0U)    /**< Minimum brightness (LED off)    */
#define BSP_LED_BRIGHTNESS_MAX       (255U)  /**< Maximum brightness (LED full on)*/
#define BSP_LED_BRIGHTNESS_DEFAULT   (128U)  /**< Default brightness after init   */
/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/

/** @defgroup BSP_LED_Exported_Functions_API Public API Functions
  * @brief    High-level LED control.  These functions delegate to the
  *           registered ::BSP_LED_ImplOps_t implementation via the Bridge.
  * @{
  */

/**
  * @brief  Register an implementation operations table with the handle.
  * @note   Must be called (directly or via a convenience wrapper) before any
  *         other API function and after @ref pImplData has been assigned.
  * @param  hled : Pointer to LED handle
  * @param  pOps : Pointer to a concrete ::BSP_LED_ImplOps_t table
  * @retval BSP_LED_Status_t
  */
BSP_LED_Status_t BSP_LED_RegisterImpl(BSP_LED_HandleTypeDef *hled,
                                       const BSP_LED_ImplOps_t *pOps);

/**
  * @brief  Initialize the LED.
  * @param  hled    : Pointer to LED handle
  * @param  pConfig : Pointer to implementation-specific configuration
  * @retval BSP_LED_Status_t
  */
BSP_LED_Status_t BSP_LED_Init(BSP_LED_HandleTypeDef *hled, const void *pConfig);

/**
  * @brief  De-initialize the LED.
  * @param  hled : Pointer to LED handle
  * @retval BSP_LED_Status_t
  */
BSP_LED_Status_t BSP_LED_DeInit(BSP_LED_HandleTypeDef *hled);

/**
  * @brief  Turn the LED on.
  * @param  hled : Pointer to LED handle
  * @retval BSP_LED_Status_t
  */
BSP_LED_Status_t BSP_LED_On(BSP_LED_HandleTypeDef *hled);

/**
  * @brief  Turn the LED off.
  * @param  hled : Pointer to LED handle
  * @retval BSP_LED_Status_t
  */
BSP_LED_Status_t BSP_LED_Off(BSP_LED_HandleTypeDef *hled);

/**
  * @brief  Toggle the LED state.
  * @param  hled : Pointer to LED handle
  * @retval BSP_LED_Status_t
  */
BSP_LED_Status_t BSP_LED_Toggle(BSP_LED_HandleTypeDef *hled);

/**
  * @brief  Set the LED brightness using PWM.
  * @note   If the backend does not support PWM, the driver falls back to
  *         GPIO on / off based on the brightness threshold.
  * @param  hled       : Pointer to LED handle
  * @param  brightness : Brightness (0 = off, 255 = full brightness)
  * @retval BSP_LED_Status_t
  */
BSP_LED_Status_t BSP_LED_SetBrightness(BSP_LED_HandleTypeDef *hled,
                                        uint8_t brightness);

/**
  * @brief  Get the current LED logical state.
  * @param  hled : Pointer to LED handle
  * @retval BSP_LED_State_t
  */
BSP_LED_State_t BSP_LED_GetState(const BSP_LED_HandleTypeDef *hled);

/**
  * @brief  Get the current LED brightness value.
  * @param  hled : Pointer to LED handle
  * @retval Brightness (0 - 255)
  */
uint8_t BSP_LED_GetBrightness(const BSP_LED_HandleTypeDef *hled);

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __BSP_LED_DRIVER_H */
