/******************************************************************************
 *
 * @file aht21_iic_bridge.c
 *
 * @par aht21_iic_bridge.h
 *
 * @brief This file implements the ::iic_driver_interface_t (software iic) 
 *        interface using the iic_hal functions. you can implements by your 
 *        own implement functions.
 * 
 * @version V1.0.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "aht21_iic_bridge.h"

#ifdef OS_SUPPORTING
// add your own OS header file
#include "FreeRTOS.h"
#include "task.h"

#endif

/* Defines -------------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

static aht21_status_t iic_init (void *ctx);
static aht21_status_t iic_deinit (void *ctx);
static aht21_status_t iic_start (void *ctx);
static aht21_status_t iic_stop (void *ctx);
static aht21_status_t iic_send_byte (void *ctx, uint8_t byte);
static aht21_status_t iic_receive_byte (void *ctx, uint8_t *byte);
static aht21_status_t iic_send_ack (void *ctx);
static aht21_status_t iic_send_no_ack (void *ctx);
static aht21_status_t iic_wait_ack (void *ctx);

#ifdef OS_SUPPORTING

static aht21_status_t iic_critical_enter (void *ctx);
static aht21_status_t iic_critical_exit (void *ctx);

#endif

/* Private variables ---------------------------------------------------------*/

/* Exported functions ---------------------------------------------------------*/


/** 
 *  @brief  Initialize a per-instance iic_driver_interface_t function table.
 *          Each call fills a separate struct — no global singleton, fully
 *          reentrant for multiple AHT21 / multiple IIC buses.
 * 
 *  @param[out] p_interface: Pointer to caller-allocated iic_driver_interface_t
 *  @param[in]  aht21_bus:   Pointer to iic_bus_t (bus GPIO configuration)
 */
void IIC_Drive_Interface_Init (iic_driver_interface_t *p_interface, iic_bus_t *aht21_bus)
{
    p_interface->context = aht21_bus;

    p_interface->pf_iic_init        = iic_init;
    p_interface->pf_iic_deinit      = iic_deinit;
    p_interface->pf_iic_start       = iic_start;
    p_interface->pf_iic_stop        = iic_stop;
    p_interface->pf_iic_send_byte   = iic_send_byte;
    p_interface->pf_iic_receive_byte = iic_receive_byte;
    p_interface->pf_iic_send_ack    = iic_send_ack;
    p_interface->pf_iic_send_no_ack = iic_send_no_ack;
    p_interface->pf_iic_wait_ack    = iic_wait_ack;
#ifdef OS_SUPPORTING
    p_interface->pf_critical_enter  = iic_critical_enter;
    p_interface->pf_critical_exit   = iic_critical_exit;
#endif
}


/* Private functions ----------------------------------------------------------*/


/** 
 *  @brief  IIC init interface
 * 
 *  @param[in] ctx: Pointer to iic_bus_t context
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_init (void *ctx)
{
    IICInit((iic_bus_t *)ctx);
    return AHT21_OK;
}

/** 
 *  @brief  IIC deinit interface
 * 
 *  @param[in] ctx: Pointer to iic_bus_t context
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_deinit (void *ctx)
{
    iic_bus_t *bus = (iic_bus_t *)ctx;
    HAL_GPIO_DeInit(bus->IIC_SDA_PORT, bus->IIC_SDA_PIN);
    HAL_GPIO_DeInit(bus->IIC_SCL_PORT, bus->IIC_SCL_PIN);
    return AHT21_OK;
}

/** 
 *  @brief  IIC start interface
 * 
 *  @param[in] ctx: Pointer to iic_bus_t context
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_start (void *ctx)
{
    IICStart((iic_bus_t *)ctx);
    return AHT21_OK;
}


/** 
 *  @brief  IIC stop interface
 * 
 *  @param[in] ctx: Pointer to iic_bus_t context
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_stop (void *ctx)
{
    IICStop((iic_bus_t *)ctx);
    return AHT21_OK;
}


/** 
 *  @brief  IIC send byte interface
 * 
 *  @param[in] ctx:  Pointer to iic_bus_t context
 *  @param[in] byte: send data
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_send_byte (void *ctx, uint8_t byte)
{
    IICSendByte((iic_bus_t *)ctx, byte);
    return AHT21_OK;
}

/** 
 *  @brief  IIC receive byte interface
 * 
 *  @param[in]  ctx:  Pointer to iic_bus_t context
 *  @param[out] byte: receive data
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_receive_byte (void *ctx, uint8_t *byte)
{
    
    if (byte == NULL)
    {
        return AHT21_ERROR_PARAMETER;
    }
    
    *byte = IICReceiveByte((iic_bus_t *)ctx);
    return AHT21_OK;
}

/** 
 *  @brief  IIC send ack interface
 * 
 *  @param[in] ctx: Pointer to iic_bus_t context
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_send_ack (void *ctx)
{
    IICSendAck((iic_bus_t *)ctx);
    return AHT21_OK;
}

/** 
 *  @brief  IIC send no ack interface
 * 
 *  @param[in] ctx: Pointer to iic_bus_t context
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_send_no_ack (void *ctx)
{
    IICSendNotAck((iic_bus_t *)ctx);
    return AHT21_OK;
}


/** 
 *  @brief  IIC wait ack interface
 * 
 *  @param[in] ctx: Pointer to iic_bus_t context
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_wait_ack (void *ctx)
{
    
    if (IICWaitAck((iic_bus_t *)ctx))
    {
        return AHT21_ERROR_TIMEOUT;
    }
    
    return AHT21_OK;
}

// if support RTOS 
#ifdef OS_SUPPORTING

/** 
 *  @brief  Software IIC enter critical
 * 
 *  @param[in] ctx: Unused (kept for interface consistency)
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_critical_enter (void *ctx)
{
    (void)ctx;  /* unused */
    taskENTER_CRITICAL();
    return AHT21_OK;
}

/** 
 *  @brief  Software IIC exit critical
 * 
 *  @param[in] ctx: Unused (kept for interface consistency)
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_critical_exit (void *ctx)
{
    (void)ctx;  /* unused */
    taskEXIT_CRITICAL();
    return AHT21_OK;
}

#endif  /* OS_SUPPORTING */


