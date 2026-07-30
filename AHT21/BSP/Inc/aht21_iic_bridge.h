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
 *  @brief  initialize the iic_driver_interface_t interface function table
 * 
 *  @param[in] aht21_bus: Pointer to iic_bus_t
 * 
 *  @return iic_driver_interface_t*
 */
iic_driver_interface_t* IIC_Drive_Interface_Init (iic_bus_t *aht21_bus);
