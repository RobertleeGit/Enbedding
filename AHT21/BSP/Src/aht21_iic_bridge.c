/******************************************************************************
 *
 * @file aht21_iic_bridge.c
 *
 * @par aht21_iic_bridge.h
 *
 * @brief This file implements the ::iic_driver_interface_t (software iic) 
 *        interface using the iic_hal functions. you can implements by you 
 *        own implement functions.
 * 
 * @version V1.0.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "aht21_iic_bridge.h"
#include "FreeRTOS.h"
#include "task.h"

/* Defines -------------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

static aht21_status_t iic_init (void);
static aht21_status_t iic_deinit (void);
static aht21_status_t iic_start (void);
static aht21_status_t iic_stop (void);
static aht21_status_t iic_send_byte (uint8_t byte);
static aht21_status_t iic_receive_byte (uint8_t *byte);
static aht21_status_t iic_send_ack (void);
static aht21_status_t iic_send_no_ack (void);
static aht21_status_t iic_wait_ack (void);

#ifdef OS_SUPPORTING

static aht21_status_t iic_critical_enter (void);
static aht21_status_t iic_critical_exit (void);

#endif

/* Private variables ---------------------------------------------------------*/

static iic_bus_t* AHT21_bus;

// iic_driver_interface_t interface function table
static const iic_driver_interface_t iic_driver = {
    .pf_iic_init = iic_init,
    .pf_iic_deinit = iic_deinit,
    .pf_iic_start = iic_start,
    .pf_iic_stop = iic_stop,
    .pf_iic_send_byte = iic_send_byte,
    .pf_iic_receive_byte = iic_receive_byte,
    .pf_iic_send_ack = iic_send_ack,
    .pf_iic_send_no_ack = iic_send_no_ack,
    .pf_iic_wait_ack = iic_wait_ack,
#ifdef OS_SUPPORTING
    .pf_critical_enter = iic_critical_enter,
    .pf_critical_exit = iic_critical_exit,
#endif
};

/* Exported functions ---------------------------------------------------------*/


/** 
 *  @brief  initialize the iic_driver_interface_t interface function table
 * 
 *  @param[in] aht21_bus: Pointer to iic_bus_t
 * 
 *  @return iic_driver_interface_t*
 */
iic_driver_interface_t* IIC_Drive_Interface_Init (iic_bus_t *aht21_bus)
{
    AHT21_bus = aht21_bus;
    return (iic_driver_interface_t*) &iic_driver;
}


/* Private functions ----------------------------------------------------------*/


/** 
 *  @brief  IIC init interface
 * 
 *  @param[in] ctx: Pointer to context
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_init (void)
{
    IICInit(AHT21_bus);
    return AHT21_OK;
}

/** 
 *  @brief  IIC deinit interface
 * 
 *  @param[in] ctx: Pointer to context
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_deinit (void)
{
    HAL_GPIO_DeInit(AHT21_bus->IIC_SDA_PORT, AHT21_bus->IIC_SDA_PIN);
    HAL_GPIO_DeInit(AHT21_bus->IIC_SCL_PORT, AHT21_bus->IIC_SCL_PIN);
    return AHT21_OK;
}

/** 
 *  @brief  IIC start interface
 * 
 *  @param[in] ctx: Pointer to context
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_start (void)
{
    IICStart(AHT21_bus);
    return AHT21_OK;
}


/** 
 *  @brief  IIC stop interface
 * 
 *  @param[in] ctx: Pointer to context
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_stop (void)
{
    IICStop(AHT21_bus);
    return AHT21_OK;
}


/** 
 *  @brief  IIC stop interface
 * 
 *  @param[in] ctx: Pointer to context
 *  @param[in] byte: send data
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_send_byte (uint8_t byte)
{
    IICSendByte(AHT21_bus, byte);
    return AHT21_OK;
}

/** 
 *  @brief  IIC receive byte interface
 * 
 *  @param[in] ctx: Pointer to context
 *  @param[out] byte: receive data
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_receive_byte (uint8_t *byte)
{
    
    if (byte == NULL)
    {
        return AHT21_ERROR_PARAMETER;
    }
    
    *byte = IICReceiveByte(AHT21_bus);
    return AHT21_OK;
}

/** 
 *  @brief  IIC send ack interface
 * 
 *  @param[in] ctx: Pointer to context
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_send_ack (void)
{
    IICSendAck(AHT21_bus);
    return AHT21_OK;
}

/** 
 *  @brief  IIC send no ack interface
 * 
 *  @param[in] ctx: Pointer to context
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_send_no_ack (void)
{
    IICSendNotAck(AHT21_bus);
    return AHT21_OK;
}


/** 
 *  @brief  IIC wait ack interface
 * 
 *  @param[in] ctx: Pointer to context
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_wait_ack (void)
{
    
    if (IICWaitAck(AHT21_bus))
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
 *  @return aht21_status_t
 */
static aht21_status_t iic_critical_enter (void)
{
    taskENTER_CRITICAL();
    return AHT21_OK;
}

/** 
 *  @brief  Software IIC exit critical
 * 
 *  @return aht21_status_t
 */
static aht21_status_t iic_critical_exit (void)
{
    taskEXIT_CRITICAL();
    return AHT21_OK;
}

#endif  /* OS_SUPPORTING */


