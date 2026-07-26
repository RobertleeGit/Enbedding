/**
  ******************************************************************************
  * @file    led_blink_pattern.c
  * @brief   LED Blink Pattern — State-Machine Implementation
  *
  *          This file implements the blink-pattern "class" defined in
  *          ::led_blink_pattern.h.  Each object runs an independent state
  *          machine that drives one LED via the ::bsp_led_driver bridge.
  *
  *          State machine:
  *          @startuml
  *          [*] --> IDLE
  *          IDLE --> ON          : Start()
  *          ON   --> OFF         : onDuration elapsed
  *          OFF  --> ON          : offDuration elapsed (not finished)
  *          OFF  --> DONE        : offDuration elapsed (count reached)
  *          DONE --> [*]
  *          @enduml
  *
  *          The upper-layer manager calls LED_Blink_Tick() periodically
  *          (e.g. from a 1 ms timer ISR or an RTOS task).  The tick function
  *          reads elapsed time via the injected ::BSP_Time_Ops_t::GetTick
  *          and transitions the state machine accordingly.
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
#include "led_blink_pattern.h"
#include <string.h>

/* Private macro -------------------------------------------------------------*/

/**
  * @brief  Parameter-validation macro (returns ::LED_BLINK_ERROR on failure).
  */
#define BLINK_ASSERT(expr)  do {                  \
  if (!(expr)) {                                  \
    return LED_BLINK_ERROR;                       \
  }                                               \
} while (0U)

/**
  * @brief  Compute elapsed ticks with uint32_t wrap-around safety.
  * @param  now  : Current tick
  * @param  prev : Previous reference tick
  * @retval Elapsed ticks
  */
#define BLINK_ELAPSED(now, prev)  ((uint32_t)((now) - (prev)))

/* -------------------------------------------------------------------------- */
/*  Static helper: compute on- / off-durations from config                      */
/* -------------------------------------------------------------------------- */

/**
  * @brief  Calculate ON-phase duration from the current config.
  * @param  pObj : Pointer to pattern object
  * @retval Duration in milliseconds
  */
static uint32_t Blink_OnDurationMs(const LED_Blink_Pattern_t *pObj)
{
  return (pObj->config.cycleMs * pObj->config.onRatio) / 100U;
}

/**
  * @brief  Calculate OFF-phase duration from the current config.
  * @param  pObj : Pointer to pattern object
  * @retval Duration in milliseconds
  */
static uint32_t Blink_OffDurationMs(const LED_Blink_Pattern_t *pObj)
{
  return pObj->config.cycleMs - Blink_OnDurationMs(pObj);
}

/* -------------------------------------------------------------------------- */
/*  Static helper: transition to a new phase & record timestamp                */
/* -------------------------------------------------------------------------- */

/**
  * @brief  Enter a new state-machine phase, snapshot the tick, and apply LED.
  * @param  pObj      : Pointer to pattern object
  * @param  newPhase  : Destination phase
  */
static void Blink_EnterPhase(LED_Blink_Pattern_t *pObj,
                              LED_Blink_Phase_t    newPhase)
{
  pObj->phase     = newPhase;
  pObj->phaseTick = pObj->pTimeOps->GetTick();

  if (newPhase == LED_BLINK_PHASE_ON) {
    (void)BSP_LED_On(pObj->hled);
  } else {
    (void)BSP_LED_Off(pObj->hled);
  }
}

/* -------------------------------------------------------------------------- */
/*  Static helper: process a single tick in the ON phase                       */
/* -------------------------------------------------------------------------- */

/**
  * @brief  Tick handler for the ON phase.
  *
  *          If the on-duration has elapsed, transitions to:
  *           - OFF phase (normal case)
  *           - DONE phase (extreme case: onRatio 100 % + counted + last blink)
  * @param  pObj : Pointer to pattern object
  */
static void Blink_TickOn(LED_Blink_Pattern_t *pObj)
{
  uint32_t now     = pObj->pTimeOps->GetTick();
  uint32_t elapsed = BLINK_ELAPSED(now, pObj->phaseTick);
  uint32_t onMs    = Blink_OnDurationMs(pObj);

  if (elapsed < onMs) {
    return;                       /* Still in ON window — nothing to do */
  }

  /*
   * ON duration elapsed.  Check whether this is the last blink in
   * counted mode with onRatio == 100 % (no OFF phase exists).
   */
  if ((pObj->config.mode == LED_BLINK_COUNTED)
      && (onMs >= pObj->config.cycleMs)
      && (pObj->blinkIdx + 1U >= pObj->config.blinkCount))
  {
    /* No OFF phase & this was the final ON — go straight to DONE */
    Blink_EnterPhase(pObj, LED_BLINK_PHASE_DONE);
    pObj->running = 0U;
    return;
  }

  /* Normal path: OFF phase follows */
  Blink_EnterPhase(pObj, LED_BLINK_PHASE_OFF);
}

/* -------------------------------------------------------------------------- */
/*  Static helper: process a single tick in the OFF phase                      */
/* -------------------------------------------------------------------------- */

/**
  * @brief  Tick handler for the OFF phase.
  *
  *          If the off-duration has elapsed, increments the blink counter
  *          and decides whether to start another cycle or finish.
  * @param  pObj : Pointer to pattern object
  */
static void Blink_TickOff(LED_Blink_Pattern_t *pObj)
{
  uint32_t now     = pObj->pTimeOps->GetTick();
  uint32_t elapsed = BLINK_ELAPSED(now, pObj->phaseTick);
  uint32_t offMs   = Blink_OffDurationMs(pObj);

  if (elapsed < offMs) {
    return;                       /* Still in OFF window — nothing to do */
  }

  /* One complete ON-OFF cycle finished */
  pObj->blinkIdx++;

  /*
   * Check completion condition for counted mode.
   */
  if ((pObj->config.mode == LED_BLINK_COUNTED)
      && (pObj->blinkIdx >= pObj->config.blinkCount))
  {
    Blink_EnterPhase(pObj, LED_BLINK_PHASE_DONE);
    pObj->running = 0U;
    return;
  }

  /* Start the next cycle */
  Blink_EnterPhase(pObj, LED_BLINK_PHASE_ON);
}

/* ========================================================================== */
/*  Public API                                                                 */
/* ========================================================================== */

/**
  * @brief  Construct / initialise a blink pattern object.
  * @param  pObj     : Pointer to caller-allocated pattern object
  * @param  hled     : Pointer to an initialised LED driver handle
  * @param  pTimeOps : Pointer to time operations vtable
  * @param  pConfig  : Pointer to blink configuration
  * @retval LED_Blink_Status_t
  */
LED_Blink_Status_t LED_Blink_Init(LED_Blink_Pattern_t      *pObj,
                                   BSP_LED_HandleTypeDef    *hled,
                                   const BSP_Time_Ops_t     *pTimeOps,
                                   const LED_Blink_Config_t *pConfig)
{
  /* ---------- Parameter validation ---------- */
  BLINK_ASSERT(pObj     != NULL);
  BLINK_ASSERT(hled     != NULL);
  BLINK_ASSERT(pTimeOps != NULL);
  BLINK_ASSERT(pConfig  != NULL);
  BLINK_ASSERT(pTimeOps->GetTick != NULL);      /* GetTick is mandatory        */

  /* Sanity-check configuration */
  BLINK_ASSERT(pConfig->cycleMs  > 0U);
  BLINK_ASSERT(pConfig->onRatio  <= LED_BLINK_ON_RATIO_MAX);

  /* ---------- Zero-initialise & assign ---------- */
  memset(pObj, 0, sizeof(LED_Blink_Pattern_t));

  pObj->hled     = hled;
  pObj->pTimeOps = pTimeOps;
  memcpy(&pObj->config, pConfig, sizeof(LED_Blink_Config_t));

  /* Start in IDLE; LED is off */
  pObj->phase   = LED_BLINK_PHASE_IDLE;
  pObj->running = 0U;
  (void)BSP_LED_Off(pObj->hled);

  return LED_BLINK_OK;
}

/**
  * @brief  Reset the pattern object to its initial (stopped) state.
  * @param  pObj : Pointer to pattern object
  * @retval LED_Blink_Status_t
  */
LED_Blink_Status_t LED_Blink_Reset(LED_Blink_Pattern_t *pObj)
{
  BLINK_ASSERT(pObj != NULL);

  pObj->phase     = LED_BLINK_PHASE_IDLE;
  pObj->phaseTick = 0U;
  pObj->blinkIdx  = 0U;
  pObj->running   = 0U;

  (void)BSP_LED_Off(pObj->hled);

  return LED_BLINK_OK;
}

/**
  * @brief  Start (or restart) the blink pattern.
  * @param  pObj : Pointer to pattern object
  * @retval LED_Blink_Status_t
  */
LED_Blink_Status_t LED_Blink_Start(LED_Blink_Pattern_t *pObj)
{
  BLINK_ASSERT(pObj         != NULL);
  BLINK_ASSERT(pObj->hled   != NULL);
  BLINK_ASSERT(pObj->pTimeOps != NULL);

  /* Reset counters */
  pObj->blinkIdx = 0U;

  /*
   * Determine the starting phase based on duty ratio.
   *   onRatio == 0   : start in OFF (no ON phase at all)
   *   onRatio == 100 : start in ON  (no OFF phase at all)
   *   otherwise      : start in ON  (normal blink cycle)
   */
  if (pObj->config.onRatio == 0U) {
    if (pObj->config.mode == LED_BLINK_COUNTED) {
      /*
       * 0 % duty + counted mode is degenerate: the LED never turns on.
       * Mark DONE immediately to avoid an infinite OFF loop.
       */
      Blink_EnterPhase(pObj, LED_BLINK_PHASE_DONE);
      pObj->running = 0U;
      return LED_BLINK_OK;
    }
    /* Continuous mode with 0 % duty: stay off forever */
    Blink_EnterPhase(pObj, LED_BLINK_PHASE_OFF);
  } else {
    Blink_EnterPhase(pObj, LED_BLINK_PHASE_ON);
  }

  pObj->running = 1U;
  return LED_BLINK_OK;
}

/**
  * @brief  Stop the blink pattern immediately.
  * @param  pObj : Pointer to pattern object
  * @retval LED_Blink_Status_t
  */
LED_Blink_Status_t LED_Blink_Stop(LED_Blink_Pattern_t *pObj)
{
  BLINK_ASSERT(pObj != NULL);

  pObj->running = 0U;
  pObj->phase   = LED_BLINK_PHASE_IDLE;

  (void)BSP_LED_Off(pObj->hled);

  return LED_BLINK_OK;
}

/**
  * @brief  State-machine tick — must be called periodically.
  * @param  pObj : Pointer to pattern object
  * @retval LED_Blink_Status_t
  */
LED_Blink_Status_t LED_Blink_Tick(LED_Blink_Pattern_t *pObj)
{
  BLINK_ASSERT(pObj         != NULL);
  BLINK_ASSERT(pObj->hled   != NULL);

  /* Fast exit: not running or no time source */
  if ((pObj->running == 0U) || (pObj->pTimeOps == NULL)) {
    return LED_BLINK_INACTIVE;
  }

  switch (pObj->phase) {
    case LED_BLINK_PHASE_ON:
      Blink_TickOn(pObj);
      break;

    case LED_BLINK_PHASE_OFF:
      Blink_TickOff(pObj);
      break;

    case LED_BLINK_PHASE_IDLE:
    case LED_BLINK_PHASE_DONE:
    default:
      /* Terminal states — nothing to do */
      break;
  }

  return (pObj->running != 0U) ? LED_BLINK_ACTIVE : LED_BLINK_INACTIVE;
}

/**
  * @brief  Change the configuration at runtime.
  * @param  pObj    : Pointer to pattern object
  * @param  pConfig : Pointer to new configuration
  * @retval LED_Blink_Status_t
  */
LED_Blink_Status_t LED_Blink_SetConfig(LED_Blink_Pattern_t      *pObj,
                                        const LED_Blink_Config_t *pConfig)
{
  BLINK_ASSERT(pObj    != NULL);
  BLINK_ASSERT(pConfig != NULL);

  /* Sanity-check configuration */
  BLINK_ASSERT(pConfig->cycleMs  > 0U);
  BLINK_ASSERT(pConfig->onRatio  <= LED_BLINK_ON_RATIO_MAX);

  memcpy(&pObj->config, pConfig, sizeof(LED_Blink_Config_t));

  return LED_BLINK_OK;
}

/**
  * @brief  Query whether the pattern is currently active.
  * @param  pObj : Pointer to pattern object
  * @retval 0 = inactive (stopped / done), 1 = active
  */
uint8_t LED_Blink_IsActive(const LED_Blink_Pattern_t *pObj)
{
  if (pObj == NULL) {
    return 0U;
  }

  return pObj->running;
}

/**
  * @brief  Query the current completed blink count.
  * @param  pObj : Pointer to pattern object
  * @retval Number of completed ON-OFF cycles since last start
  */
uint32_t LED_Blink_GetCount(const LED_Blink_Pattern_t *pObj)
{
  if (pObj == NULL) {
    return 0U;
  }

  return pObj->blinkIdx;
}
