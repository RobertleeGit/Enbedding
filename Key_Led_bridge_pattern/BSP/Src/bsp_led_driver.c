/**
  ******************************************************************************
  * @file    bsp_led_driver.c
  * @brief   LED Driver — Bridge Pattern Abstraction Layer
  *
  *          This file implements the high-level LED driver API.  All hardware
  *          operations are delegated through the ::BSP_LED_ImplOps_t vtable,
  *          keeping this module completely decoupled from any specific hardware
  *          library or MCU platform.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "bsp_led_driver.h"
#include <stdio.h>

/* Private macro -------------------------------------------------------------*/

/**
  * @brief  Parameter-validation macro (HAL-style).
  *         Returns ::BSP_LED_ERROR if the expression evaluates to false.
  */
#define BSP_LED_ASSERT(expr)  do {                 \
  if (!(expr)) {                                   \
    return BSP_LED_ERROR;                          \
  }                                                \
} while (0U)

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Register a concrete implementation operations table with the handle.
  * @param  hled : Pointer to LED handle
  * @param  pOps : Pointer to a concrete ::BSP_LED_ImplOps_t table
  * @retval BSP_LED_Status_t
  */
BSP_LED_Status_t BSP_LED_RegisterImpl(BSP_LED_HandleTypeDef *hled,
                                       const BSP_LED_ImplOps_t *pOps)
{
  /* ---------- Parameter check ---------- */
  BSP_LED_ASSERT(hled != NULL);
  BSP_LED_ASSERT(pOps != NULL);
  BSP_LED_ASSERT(hled->pImplData != NULL);

  /* Required operations must be provided */
  BSP_LED_ASSERT(pOps->On     != NULL);
  BSP_LED_ASSERT(pOps->Off    != NULL);
  BSP_LED_ASSERT(pOps->Toggle != NULL);

  /* ---------- Register the implementation ---------- */
  hled->pOps = pOps;

  return BSP_LED_OK;
}

/**
  * @brief  Initialize the LED with the registered implementation.
  * @param  hled    : Pointer to LED handle
  * @param  pConfig : Pointer to implementation-specific configuration
  * @retval BSP_LED_Status_t
  */
BSP_LED_Status_t BSP_LED_Init(BSP_LED_HandleTypeDef *hled, const void *pConfig)
{
  BSP_LED_Status_t status;

  /* ---------- Parameter check ---------- */
  BSP_LED_ASSERT(hled         != NULL);
  BSP_LED_ASSERT(hled->pOps   != NULL);
  BSP_LED_ASSERT(hled->pImplData != NULL);

  /* ---------- Reset handle state ---------- */
  hled->state      = BSP_LED_STATE_OFF;
  hled->brightness = BSP_LED_BRIGHTNESS_DEFAULT;

  /* ---------- Delegate to implementation ---------- */
  if (hled->pOps->Init != NULL) {
    status = hled->pOps->Init(hled, pConfig);
  } else {
    status = BSP_LED_OK;          /* Init is optional */
  }

  return status;
}

/**
  * @brief  De-initialize the LED.
  * @param  hled : Pointer to LED handle
  * @retval BSP_LED_Status_t
  */
BSP_LED_Status_t BSP_LED_DeInit(BSP_LED_HandleTypeDef *hled)
{
  BSP_LED_Status_t status;

  /* ---------- Parameter check ---------- */
  BSP_LED_ASSERT(hled         != NULL);
  BSP_LED_ASSERT(hled->pOps   != NULL);
  BSP_LED_ASSERT(hled->pImplData != NULL);

  /* ---------- Delegate to implementation ---------- */
  if (hled->pOps->DeInit != NULL) {
    status = hled->pOps->DeInit(hled);
  } else {
    status = BSP_LED_OK;
  }

  /* ---------- Reset handle state ---------- */
  hled->state      = BSP_LED_STATE_OFF;
  hled->brightness = 0U;

  return status;
}

/**
  * @brief  Turn the LED on.
  * @param  hled : Pointer to LED handle
  * @retval BSP_LED_Status_t
  */
BSP_LED_Status_t BSP_LED_On(BSP_LED_HandleTypeDef *hled)
{
  BSP_LED_Status_t status;

  /* ---------- Parameter check ---------- */
  BSP_LED_ASSERT(hled              != NULL);
  BSP_LED_ASSERT(hled->pOps        != NULL);
  BSP_LED_ASSERT(hled->pOps->On    != NULL);
  BSP_LED_ASSERT(hled->pImplData   != NULL);

  /* ---------- Delegate to implementation ---------- */
  status = hled->pOps->On(hled);

  /* ---------- Update logical state ---------- */
  if (status == BSP_LED_OK) {
    hled->state      = BSP_LED_STATE_ON;
    hled->brightness = BSP_LED_BRIGHTNESS_MAX;
  }

  return status;
}

/**
  * @brief  Turn the LED off.
  * @param  hled : Pointer to LED handle
  * @retval BSP_LED_Status_t
  */
BSP_LED_Status_t BSP_LED_Off(BSP_LED_HandleTypeDef *hled)
{
  BSP_LED_Status_t status;

  /* ---------- Parameter check ---------- */
  BSP_LED_ASSERT(hled              != NULL);
  BSP_LED_ASSERT(hled->pOps        != NULL);
  BSP_LED_ASSERT(hled->pOps->Off   != NULL);
  BSP_LED_ASSERT(hled->pImplData   != NULL);

  /* ---------- Delegate to implementation ---------- */
  status = hled->pOps->Off(hled);

  /* ---------- Update logical state ---------- */
  if (status == BSP_LED_OK) {
    hled->state      = BSP_LED_STATE_OFF;
    hled->brightness = BSP_LED_BRIGHTNESS_MIN;
  }

  return status;
}

/**
  * @brief  Toggle the LED state.
  * @param  hled : Pointer to LED handle
  * @retval BSP_LED_Status_t
  */
BSP_LED_Status_t BSP_LED_Toggle(BSP_LED_HandleTypeDef *hled)
{
  BSP_LED_Status_t status;

  /* ---------- Parameter check ---------- */
  BSP_LED_ASSERT(hled               != NULL);
  BSP_LED_ASSERT(hled->pOps         != NULL);
  BSP_LED_ASSERT(hled->pOps->Toggle != NULL);
  BSP_LED_ASSERT(hled->pImplData    != NULL);

  /* ---------- Delegate to implementation ---------- */
  status = hled->pOps->Toggle(hled);

  /* ---------- Update logical state ---------- */
  if (status == BSP_LED_OK) {
    if (hled->state == BSP_LED_STATE_ON) {
      hled->state      = BSP_LED_STATE_OFF;
      hled->brightness = BSP_LED_BRIGHTNESS_MIN;
    } else {
      hled->state      = BSP_LED_STATE_ON;
      hled->brightness = BSP_LED_BRIGHTNESS_MAX;
    }
  }

  return status;
}

/**
  * @brief  Set the LED brightness using PWM.
  * @note   Falls back to GPIO on / off if the implementation does not provide
  *         a SetBrightness function.
  * @param  hled       : Pointer to LED handle
  * @param  brightness : Brightness (0 = off, 255 = full brightness)
  * @retval BSP_LED_Status_t
  */
BSP_LED_Status_t BSP_LED_SetBrightness(BSP_LED_HandleTypeDef *hled,
                                        uint8_t brightness)
{
  BSP_LED_Status_t status;

  /* ---------- Parameter check ---------- */
  BSP_LED_ASSERT(hled            != NULL);
  BSP_LED_ASSERT(hled->pOps      != NULL);
  BSP_LED_ASSERT(hled->pImplData != NULL);

  /* ---------- Fallback: no PWM support ---------- */
  if (hled->pOps->SetBrightness == NULL) {
    if (brightness == BSP_LED_BRIGHTNESS_MIN) {
      return BSP_LED_Off(hled);
    } else {
      return BSP_LED_On(hled);
    }
  }

  /* ---------- Delegate to implementation ---------- */
  status = hled->pOps->SetBrightness(hled, brightness);

  /* ---------- Update logical state ---------- */
  if (status == BSP_LED_OK) {
    hled->brightness = brightness;
    if (brightness > BSP_LED_BRIGHTNESS_MIN) {
      hled->state = BSP_LED_STATE_ON;
    } else {
      hled->state = BSP_LED_STATE_OFF;
    }
  }

  return status;
}

/**
  * @brief  Get the current LED logical state.
  * @param  hled : Pointer to LED handle
  * @retval BSP_LED_State_t
  */
BSP_LED_State_t BSP_LED_GetState(const BSP_LED_HandleTypeDef *hled)
{
  if (hled == NULL) {
    return BSP_LED_STATE_OFF;
  }

  return hled->state;
}

/**
  * @brief  Get the current LED brightness value.
  * @param  hled : Pointer to LED handle
  * @retval Brightness (0 - 255)
  */
uint8_t BSP_LED_GetBrightness(const BSP_LED_HandleTypeDef *hled)
{
  if (hled == NULL) {
    return 0U;
  }

  return hled->brightness;
}








