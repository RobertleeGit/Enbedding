/******************************************************************************
 *
 * @file aht21_iic_bridge.c
 *
 * @par iic_hal.h
 *		ec_bsp_aht21_driver.h
 *
 * @brief This file implements the ::iic_driver_interface_t interface using the
 *        iic_hal function.
 *
 * @version V1.0.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#include "iic_hal.h"
#include "bsp_aht21_driver.h"


/** 
 *  @brief  Initialize a per-instance iic_driver_interface_t function table.
 *          Each call fills a separate struct, enabling multiple AHT21 sensors
 *          on different IIC buses to coexist.
 * 
 *  @param[out] p_interface: Pointer to caller-allocated iic_driver_interface_t
 *  @param[in]  aht21_bus:   Pointer to iic_bus_t (bus GPIO configuration)
 */
void IIC_Drive_Interface_Init (iic_driver_interface_t *p_interface, iic_bus_t *aht21_bus);
