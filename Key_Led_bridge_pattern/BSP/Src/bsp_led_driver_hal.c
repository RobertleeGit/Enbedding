/**
  ******************************************************************************
  * @file    bsp_led_driver_hal.c
  * @brief   LED Driver — HAL Concrete Implementation
  *
  *          This file implements the ::BSP_LED_ImplOps_t interface using the
  *          STM32F4xx HAL library.  It is the "concrete implementation" side
  *          of the Bridge Pattern.
  *
  *          Key responsibilities:
  *           - GPIO write / toggle for ON / OFF / Toggle operations
  *           - TIM PWM duty-cycle control for brightness
  *           - Active-level handling (active-high vs active-low LEDs)
  *
  *          To switch to a different backend (LL, StdPeriph, another MCU):
  *            1. Implement a new file that provides the same ::BSP_LED_ImplOps_t.
  *            2. Call ::BSP_LED_RegisterImpl with the new ops table.
  *          No changes to ::bsp_led_driver.c or application code are required.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "bsp_led_driver_hal.h"
#include <string.h>

/* Private macro -------------------------------------------------------------*/

/**
  * @brief  Compute the physical GPIO output level for a desired logical state,
  *         taking the active-level polarity into account.
  *
  *         active-high : ON  -> GPIO_PIN_SET,   OFF -> GPIO_PIN_RESET
  *         active-low  : ON  -> GPIO_PIN_RESET, OFF -> GPIO_PIN_SET
  */
#define HAL_LED_GPIO_LEVEL(pData, logicalOn) \
  (((pData)->activeLevel == BSP_LED_ACTIVE_LOW) ? \
   ((logicalOn) ? GPIO_PIN_RESET : GPIO_PIN_SET) : \
   ((logicalOn) ? GPIO_PIN_SET   : GPIO_PIN_RESET))

/* Private function prototypes -----------------------------------------------*/

static BSP_LED_Status_t HAL_LED_Init(BSP_LED_HandleTypeDef *hled,
                                      const void *pConfig);
static BSP_LED_Status_t HAL_LED_DeInit(BSP_LED_HandleTypeDef *hled);
static BSP_LED_Status_t HAL_LED_On(BSP_LED_HandleTypeDef *hled);
static BSP_LED_Status_t HAL_LED_Off(BSP_LED_HandleTypeDef *hled);
static BSP_LED_Status_t HAL_LED_Toggle(BSP_LED_HandleTypeDef *hled);
static BSP_LED_Status_t HAL_LED_SetBrightness(BSP_LED_HandleTypeDef *hled,
                                               uint8_t brightness);

/* Private variables ---------------------------------------------------------*/

/**
  * @brief  Read-only HAL implementation operations table.
  *
  *          Registered with each ::BSP_LED_HandleTypeDef at init time.
  *          The ::BSP_LED_ImplOps_t structure acts as a manual vtable,
  *          providing runtime polymorphism without C++ overhead.
  */
static const BSP_LED_ImplOps_t HAL_LED_Ops = {
  .Init          = HAL_LED_Init,
  .DeInit        = HAL_LED_DeInit,
  .On            = HAL_LED_On,
  .Off           = HAL_LED_Off,
  .Toggle        = HAL_LED_Toggle,
  .SetBrightness = HAL_LED_SetBrightness,
};

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  One-shot HAL initialization convenience function.
  * @param  hled    : Pointer to LED handle
  * @param  pData   : Pointer to HAL data buffer
  * @param  pConfig : Pointer to HAL configuration
  * @retval BSP_LED_Status_t
  */
BSP_LED_Status_t BSP_LED_HAL_Init(BSP_LED_HandleTypeDef *hled,
                                   BSP_LED_HAL_Data_t    *pData,
                                   const BSP_LED_HAL_Config_t *pConfig)
{
  if ((hled == NULL) || (pData == NULL) || (pConfig == NULL)) {
    return BSP_LED_ERROR;
  }

  /* Zero-initialize both structures */
  memset(hled,  0, sizeof(BSP_LED_HandleTypeDef));
  memset(pData, 0, sizeof(BSP_LED_HAL_Data_t));

  /* Bind the HAL data to the handle */
  hled->pImplData = pData;

  /* Register the HAL implementation */
  (void)BSP_LED_RegisterImpl(hled, &HAL_LED_Ops);

  /* Delegate the actual hardware init */
  return BSP_LED_Init(hled, pConfig);
}

/**
  * @brief  Obtain the HAL implementation operations table.
  * @retval Pointer to the read-only HAL ops table.
  */
const BSP_LED_ImplOps_t *BSP_LED_HAL_GetOps(void)
{
  return &HAL_LED_Ops;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initialize LED hardware peripherals via HAL.
  *
  *          Stores the configuration in the HAL data area and puts the LED
  *          in a known-off state.  If a PWM timer is provided, starts the
  *          PWM channel.
  *
  * @param  hled    : Pointer to LED handle
  * @param  pConfig : Pointer to ::BSP_LED_HAL_Config_t
  * @retval BSP_LED_Status_t
  */
static BSP_LED_Status_t HAL_LED_Init(BSP_LED_HandleTypeDef *hled,
                                      const void *pConfig)
{
  BSP_LED_HAL_Data_t        *pData;
  const BSP_LED_HAL_Config_t *pCfg;

  /* Safety guards */
  if ((hled == NULL) || (pConfig == NULL)) {
    return BSP_LED_ERROR;
  }

  pCfg  = (const BSP_LED_HAL_Config_t *)pConfig;
  pData = (BSP_LED_HAL_Data_t *)(hled->pImplData);

  /* Validate essential configuration */
  if ((pCfg->port == NULL) || (pCfg->pin == 0x0000U)) {
    return BSP_LED_ERROR;
  }

  /* ---------- Store hardware binding ---------- */
  pData->port        = pCfg->port;
  pData->pin         = pCfg->pin;
  pData->activeLevel = pCfg->activeLevel;
  pData->htim        = pCfg->htim;
  pData->timChannel  = pCfg->timChannel;

  /* ---------- Force LED to OFF state ---------- */
  HAL_GPIO_WritePin(pData->port,
                    pData->pin,
                    HAL_LED_GPIO_LEVEL(pData, 0U));

  /* ---------- Start PWM if a timer is configured ---------- */
  if (pData->htim != NULL) {
    HAL_TIM_PWM_Start(pData->htim, pData->timChannel);
  }

  return BSP_LED_OK;
}

/**
  * @brief  De-initialize LED hardware peripherals.
  * @param  hled : Pointer to LED handle
  * @retval BSP_LED_Status_t
  */
static BSP_LED_Status_t HAL_LED_DeInit(BSP_LED_HandleTypeDef *hled)
{
  BSP_LED_HAL_Data_t *pData;

  if (hled == NULL) {
    return BSP_LED_ERROR;
  }

  pData = (BSP_LED_HAL_Data_t *)(hled->pImplData);

  /* Turn off the LED before shutting down peripherals */
  HAL_GPIO_WritePin(pData->port,
                    pData->pin,
                    HAL_LED_GPIO_LEVEL(pData, 0U));

  /* Stop PWM channel if it was running */
  if (pData->htim != NULL) {
    HAL_TIM_PWM_Stop(pData->htim, pData->timChannel);
  }

  /* Clear out the configuration */
  memset(pData, 0, sizeof(BSP_LED_HAL_Data_t));

  return BSP_LED_OK;
}

/**
  * @brief  Turn LED on — HAL GPIO write.
  * @param  hled : Pointer to LED handle
  * @retval BSP_LED_Status_t
  */
static BSP_LED_Status_t HAL_LED_On(BSP_LED_HandleTypeDef *hled)
{
  BSP_LED_HAL_Data_t *pData;

  if (hled == NULL) {
    return BSP_LED_ERROR;
  }

  pData = (BSP_LED_HAL_Data_t *)(hled->pImplData);

  HAL_GPIO_WritePin(pData->port,
                    pData->pin,
                    HAL_LED_GPIO_LEVEL(pData, 1U));

  return BSP_LED_OK;
}

/**
  * @brief  Turn LED off — HAL GPIO write.
  * @param  hled : Pointer to LED handle
  * @retval BSP_LED_Status_t
  */
static BSP_LED_Status_t HAL_LED_Off(BSP_LED_HandleTypeDef *hled)
{
  BSP_LED_HAL_Data_t *pData;

  if (hled == NULL) {
    return BSP_LED_ERROR;
  }

  pData = (BSP_LED_HAL_Data_t *)(hled->pImplData);

  HAL_GPIO_WritePin(pData->port,
                    pData->pin,
                    HAL_LED_GPIO_LEVEL(pData, 0U));

  return BSP_LED_OK;
}

/**
  * @brief  Toggle LED — HAL GPIO toggle.
  * @param  hled : Pointer to LED handle
  * @retval BSP_LED_Status_t
  */
static BSP_LED_Status_t HAL_LED_Toggle(BSP_LED_HandleTypeDef *hled)
{
  BSP_LED_HAL_Data_t *pData;

  if (hled == NULL) {
    return BSP_LED_ERROR;
  }

  pData = (BSP_LED_HAL_Data_t *)(hled->pImplData);

  HAL_GPIO_TogglePin(pData->port, pData->pin);

  return BSP_LED_OK;
}

/**
  * @brief  Set LED brightness via TIM PWM duty-cycle control.
  *
  *          For active-low LEDs the duty cycle is inverted so that
  *          brightness=0 produces a fully-off LED and brightness=255
  *          produces a fully-on LED.
  *
  * @param  hled       : Pointer to LED handle
  * @param  brightness : Desired brightness [0 … 255]
  * @retval BSP_LED_Status_t
  */
static BSP_LED_Status_t HAL_LED_SetBrightness(BSP_LED_HandleTypeDef *hled,
                                               uint8_t brightness)
{
  BSP_LED_HAL_Data_t *pData;
  uint32_t            pulse;
  uint32_t            period;

  if (hled == NULL) {
    return BSP_LED_ERROR;
  }

  pData = (BSP_LED_HAL_Data_t *)(hled->pImplData);

  /*
   * No PWM timer configured — delegate to GPIO on/off.
   * (The caller in bsp_led_driver.c also performs this fallback,
   *  but we duplicate it here so the HAL backend is self-sufficient.)
   */
  if (pData->htim == NULL) {
    if (brightness == 0U) {
      HAL_GPIO_WritePin(pData->port, pData->pin,
                        HAL_LED_GPIO_LEVEL(pData, 0U));
    } else {
      HAL_GPIO_WritePin(pData->port, pData->pin,
                        HAL_LED_GPIO_LEVEL(pData, 1U));
    }
    return BSP_LED_OK;
  }

  /* ---------- Read current PWM period (ARR + 1) ---------- */
  period = __HAL_TIM_GET_AUTORELOAD(pData->htim) + 1U;

  /*
   * Calculate the compare (CCR) value.
   *
   *   active-high : pulse = (brightness / 255) * period
   *   active-low  : pulse = ((255 - brightness) / 255) * period
   */
  if (pData->activeLevel == BSP_LED_ACTIVE_LOW) {
    pulse = ((uint32_t)(255U - brightness) * period) / 255U;
  } else {
    pulse = ((uint32_t)brightness * period) / 255U;
  }

  /* Saturate to period range */
  if (pulse >= period) {
    pulse = period - 1U;
  }

  /* Write the compare register */
  __HAL_TIM_SET_COMPARE(pData->htim, pData->timChannel, pulse);

  return BSP_LED_OK;
}