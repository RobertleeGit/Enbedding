/**
  ******************************************************************************
  * @file    led_manager.c
  * @brief   LED Manager — Message-Queue-Driven Object Manager
  *
  *          Implementation of the LED manager as described in led_manager.h.
  *
  *          Internal flow:
  *          @startuml
  *          App → LED_Mgr_SendCmd → cmdQueue
  *          LED_Mgr_Task (loop):
  *            dequeue cmd
  *            switch(cmd.cmdId):
  *              START_BLINK → init pattern → create ctrl task
  *              STOP        → delete ctrl task → LED off
  *              SET_CONFIG  → update pattern config
  *          Ctrl task per LED (loop):
  *            LED_Blink_Tick() → osDelay(1)
  *            if done → self-delete
  *          @enduml
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
#include "led_manager.h"
#include <string.h>

/* Private macro -------------------------------------------------------------*/

/**
  * @brief  Parameter-validation macro.
  */
#define MGR_ASSERT(expr)  do { if (!(expr)) { return 1U; } } while (0U)

/* Private variables ---------------------------------------------------------*/

static const BSP_OS_Ops_t  *gpOsOps   = NULL;   /**< Registered OS bridge      */
static BSP_OS_QueueHandle_t gCmdQueue = NULL;   /**< Command message queue      */
static BSP_OS_TaskHandle_t  gMgrTask  = NULL;   /**< Manager task handle        */

static LED_Mgr_LedObj_t    *gpLedPool     = NULL; /**< LED object array          */
static uint8_t              gLedPoolSize  = 0U;    /**< LED pool count           */
static uint32_t             gCtrlTaskStack = 0U;   /**< Ctrl task stack words    */
static uint32_t             gCtrlTaskPrio  = 0U;   /**< Ctrl task priority       */

/* Private function prototypes -----------------------------------------------*/

static void LED_Mgr_Task(void *arg);
static void LED_Mgr_CtrlTask(void *arg);
static LED_Mgr_LedObj_t *LED_Mgr_FindLed(uint8_t ledId);
static uint8_t LED_Mgr_StartBlink(LED_Mgr_Cmd_t *pCmd);
static uint8_t LED_Mgr_StopLed(LED_Mgr_Cmd_t *pCmd);

/* ========================================================================== */
/*  Control Task — runs the blink state machine for one LED                    */
/* ========================================================================== */

/**
  * @brief  Per-LED control task — ticks the pattern object until completion.
  *
  *          Created by the manager on START_BLINK; self-deletes when the
  *          pattern finishes or is stopped by the manager.
  *
  * @param  arg : Pointer to ::LED_Mgr_LedObj_t
  */
static void LED_Mgr_CtrlTask(void *arg)
{
  LED_Mgr_LedObj_t *pLed = (LED_Mgr_LedObj_t *)arg;

  if ((pLed == NULL) || (gpOsOps == NULL)) {
    gpOsOps->TaskDelete(NULL);
    return;
  }

  /*
   * Main tick loop.
   * The pattern's IsActive / running flag signals completion.
   * The manager may also forcibly delete this task (STOP command).
   */
  while (pLed->mode == LED_MGR_MODE_BLINK) {
    switch (pLed->mode) {
      case LED_MGR_MODE_BLINK:
        (void)LED_Blink_Tick(&pLed->pattern.blink);

        /* Check if the pattern has finished */
        if (!LED_Blink_IsActive(&pLed->pattern.blink)) {
          (void)BSP_LED_Off(pLed->hled);
          pLed->mode       = LED_MGR_MODE_OFF;

          /* Notify the caller that this LED has finished */
          if (pLed->notifyQueue != NULL) {
            (void)gpOsOps->QueueSend(pLed->notifyQueue,
                                      &pLed->id, 0U);
            pLed->notifyQueue = NULL;
          }

          pLed->taskHandle = NULL;
          gpOsOps->TaskDelete(NULL);   /* Self-delete */
          return;
        }
        break;

      default:
        /* Mode changed while running — exit loop */
        break;
    }

    gpOsOps->DelayMs(1U);
  }

  /* Clean exit (mode changed externally) */
  pLed->taskHandle = NULL;
  gpOsOps->TaskDelete(NULL);
}

/* ========================================================================== */
/*  Internal helpers                                                           */
/* ========================================================================== */

/**
  * @brief  Find a LED object by ID.
  * @param  ledId : LED identifier
  * @retval Pointer to ::LED_Mgr_LedObj_t or NULL
  */
static LED_Mgr_LedObj_t *LED_Mgr_FindLed(uint8_t ledId)
{
  uint8_t i;

  if (gpLedPool == NULL) {
    return NULL;
  }

  for (i = 0U; i < gLedPoolSize; i++) {
    if (gpLedPool[i].id == ledId) {
      return &gpLedPool[i];
    }
  }

  return NULL;
}

/**
  * @brief  Start a blink pattern on a LED.
  * @param  pCmd : Pointer to command (must be START_BLINK)
  * @retval 0 on success, non-zero on error
  */
static uint8_t LED_Mgr_StartBlink(LED_Mgr_Cmd_t *pCmd)
{
  LED_Mgr_LedObj_t *pLed;

  MGR_ASSERT(pCmd != NULL);

  pLed = LED_Mgr_FindLed(pCmd->ledId);
  MGR_ASSERT(pLed           != NULL);
  MGR_ASSERT(pLed->hled     != NULL);
  MGR_ASSERT(pLed->pTimeOps != NULL);

  /* If already running, stop first */
  if (pLed->taskHandle != NULL) {
    gpOsOps->TaskDelete(pLed->taskHandle);
    pLed->taskHandle = NULL;
  }

  /* Store completion notification target */
  pLed->notifyQueue = pCmd->notifyQueue;

  /* Initialise the blink pattern object */
  (void)LED_Blink_Init(&pLed->pattern.blink,
                        pLed->hled,
                        pLed->pTimeOps,
                        &pCmd->data.blinkCfg);

  /* Start the state machine */
  (void)LED_Blink_Start(&pLed->pattern.blink);
  pLed->mode = LED_MGR_MODE_BLINK;

  /* Create the control task */
  pLed->taskHandle = gpOsOps->TaskCreate(
      LED_Mgr_CtrlTask,
      "ledCtrl",
      gCtrlTaskStack,
      (void *)pLed,
      gCtrlTaskPrio);

  if (pLed->taskHandle == NULL) {
    /* Task creation failed — clean up */
    (void)LED_Blink_Stop(&pLed->pattern.blink);
    (void)BSP_LED_Off(pLed->hled);
    pLed->mode = LED_MGR_MODE_OFF;
    return 1U;
  }

  return 0U;
}

/**
  * @brief  Stop a LED immediately.
  * @param  pCmd : Pointer to command (must be STOP)
  * @retval 0 on success, non-zero on error
  */
static uint8_t LED_Mgr_StopLed(LED_Mgr_Cmd_t *pCmd)
{
  LED_Mgr_LedObj_t *pLed;

  MGR_ASSERT(pCmd != NULL);

  pLed = LED_Mgr_FindLed(pCmd->ledId);
  MGR_ASSERT(pLed       != NULL);
  MGR_ASSERT(pLed->hled != NULL);

  /* Terminate the control task if one is running */
  if (pLed->taskHandle != NULL) {
    gpOsOps->TaskDelete(pLed->taskHandle);
    pLed->taskHandle = NULL;
  }

  /* Turn off the LED hardware */
  (void)BSP_LED_Off(pLed->hled);
  pLed->mode = LED_MGR_MODE_OFF;

  return 0U;
}

/* ========================================================================== */
/*  Manager Task — command dispatch loop                                       */
/* ========================================================================== */

/**
  * @brief  Main manager task — dequeues commands and dispatches them.
  * @param  arg : Unused
  */
static void LED_Mgr_Task(void *arg)
{
  LED_Mgr_Cmd_t cmd;
  (void)arg;

  while (1) {
    /* Block until a command arrives */
    if (gpOsOps->QueueReceive(gCmdQueue, &cmd, BSP_OS_WAIT_FOREVER) == 0U) {
      continue;   /* Should not happen with infinite timeout */
    }

    switch (cmd.cmdId) {
      case LED_MGR_CMD_START_BLINK:
        (void)LED_Mgr_StartBlink(&cmd);
        break;

      case LED_MGR_CMD_STOP:
        (void)LED_Mgr_StopLed(&cmd);
        break;

      case LED_MGR_CMD_SET_CONFIG:
        {
          LED_Mgr_LedObj_t *pLed = LED_Mgr_FindLed(cmd.ledId);

          if ((pLed != NULL) && (pLed->mode == LED_MGR_MODE_BLINK)) {
            (void)LED_Blink_SetConfig(&pLed->pattern.blink,
                                       &cmd.data.blinkCfg);
          }
        }
        break;

      default:
        /* Unknown command — ignored */
        break;
    }
  }
}

/* ========================================================================== */
/*  Public API                                                                 */
/* ========================================================================== */

/**
  * @brief  Register an LED handle with the manager.
  * @param  ledId : Logical LED ID
  * @param  hled  : Pointer to an initialised LED driver handle
  * @retval 0 on success, non-zero on failure
  */
uint8_t LED_Mgr_RegisterLed(LED_Mgr_LedObj_t     *pPool,
                            uint8_t               poolSize,
                            uint8_t               ledId,
                            BSP_LED_HandleTypeDef *hled)
{
  MGR_ASSERT(pPool  != NULL);
  MGR_ASSERT(ledId  <  poolSize);
  MGR_ASSERT(hled   != NULL);
  MGR_ASSERT(pPool[ledId].hled == NULL);  /* Not already registered */

  pPool[ledId].id       = ledId;
  pPool[ledId].hled     = hled;
  pPool[ledId].mode     = LED_MGR_MODE_OFF;
  pPool[ledId].pTimeOps = NULL;   /* Set during Init */

  return 0U;
}

/**
  * @brief  Initialise the LED manager.
  * @param  pInit : Pointer to manager configuration
  * @retval 0 on success, non-zero on failure
  */
uint8_t LED_Mgr_Init(const LED_Mgr_Init_t *pInit)
{
  MGR_ASSERT(pInit            != NULL);
  MGR_ASSERT(pInit->pOsOps    != NULL);
  MGR_ASSERT(pInit->pTimeOps  != NULL);
  MGR_ASSERT(pInit->pLedPool  != NULL);
  MGR_ASSERT(pInit->ledPoolSize > 0U);
  MGR_ASSERT(pInit->cmdQueueLen > 0U);

  /* Store global references */
  gpOsOps         = pInit->pOsOps;
  gpLedPool       = pInit->pLedPool;
  gLedPoolSize    = pInit->ledPoolSize;
  gCtrlTaskStack  = pInit->ctrlTaskStack;
  gCtrlTaskPrio   = pInit->ctrlTaskPrio;

  /* Inject time service into each registered LED */
  for (uint8_t i = 0U; i < gLedPoolSize; i++) {
    if (gpLedPool[i].hled != NULL) {
      gpLedPool[i].pTimeOps = pInit->pTimeOps;
    }
  }

  /* Create the command queue */
  gCmdQueue = gpOsOps->QueueCreate(sizeof(LED_Mgr_Cmd_t),
                                    pInit->cmdQueueLen);
  MGR_ASSERT(gCmdQueue != NULL);

  /* Create the manager task */
  gMgrTask = gpOsOps->TaskCreate(LED_Mgr_Task,
                                  "ledMgr",
                                  pInit->ctrlTaskStack,
                                  NULL,
                                  pInit->ctrlTaskPrio);
  MGR_ASSERT(gMgrTask != NULL);

  return 0U;
}

/**
  * @brief  Send a control command to the manager (non-blocking).
  * @param  pCmd : Pointer to ::LED_Mgr_Cmd_t
  * @retval 0 on success, non-zero on failure
  */
uint8_t LED_Mgr_SendCmd(const LED_Mgr_Cmd_t *pCmd)
{
  MGR_ASSERT(pCmd      != NULL);
  MGR_ASSERT(gCmdQueue != NULL);

  if (gpOsOps->QueueSend(gCmdQueue, pCmd, 0U) == 0U) {
    return 1U;      /* Queue full */
  }

  return 0U;
}

/**
  * @brief  Stop all LEDs and shut down the manager.
  * @retval 0 on success
  */
uint8_t LED_Mgr_DeInit(void)
{
  uint8_t i;

  if (gpOsOps == NULL) {
    return 1U;
  }

  /* Stop all running LED control tasks */
  for (i = 0U; i < gLedPoolSize; i++) {
    if (gpLedPool[i].taskHandle != NULL) {
      gpOsOps->TaskDelete(gpLedPool[i].taskHandle);
      gpLedPool[i].taskHandle = NULL;
    }
    (void)BSP_LED_Off(gpLedPool[i].hled);
    gpLedPool[i].mode = LED_MGR_MODE_OFF;
  }

  /* Kill the manager task */
  if (gMgrTask != NULL) {
    gpOsOps->TaskDelete(gMgrTask);
    gMgrTask = NULL;
  }

  /* Clear globals */
  gCmdQueue = NULL;
  gpOsOps   = NULL;

  return 0U;
}

/**
  * @brief  Query a LED's current operating mode.
  * @param  ledId : LED ID
  * @retval ::LED_Mgr_Mode_t
  */
LED_Mgr_Mode_t LED_Mgr_GetMode(uint8_t ledId)
{
  LED_Mgr_LedObj_t *pLed = LED_Mgr_FindLed(ledId);

  if (pLed == NULL) {
    return LED_MGR_MODE_OFF;
  }

  return pLed->mode;
}

/**
  * @brief  Query whether a LED is currently active.
  * @param  ledId : LED ID
  * @retval 1 = active, 0 = inactive / invalid
  */
uint8_t LED_Mgr_IsActive(uint8_t ledId)
{
  LED_Mgr_LedObj_t *pLed = LED_Mgr_FindLed(ledId);

  if (pLed == NULL) {
    return 0U;
  }

  return (pLed->mode != LED_MGR_MODE_OFF) ? 1U : 0U;
}
