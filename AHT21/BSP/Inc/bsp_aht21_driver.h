/******************************************************************************
 *
 * @file bsp_aht21_driver.h
 *
 * @par stdint.h
 *
 * @brief Provide the HAL APIs of AHT21 and corresponding opetions.
 *
 * @version V1.0.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __BSP_AHT21_DRIVER_H
#define __BSP_AHT21_DRIVER_H
/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>

/* Defines -------------------------------------------------------------------*/
#define OS_SUPPORTING   // support the operating system
#define SOFTWARE_IIC    // Implemented using software IIC

#define AHT21_READ_ADD                      0x71            // read address
#define AHT21_WRITE_ADD                     0x70            // write address
#define AHT21_MAX_WAITTING_TIME             80              // Max waitting time 80 ms

// AHT21 command
#define AHT21_CMD_INIT                      0xBE            // initialized cmd
#define AHT21_CMD_TRIGGER_MEASUREMENT       0xAC            // start measuring temp. and humi. cmd
#define AHT21_CMD_RESET                     0xBA            // software reset cmd




// CRC marco
#define CRC8_POLYNOMIAL                     0x31            // CRC-8 polynomial
#define CRC8_INTIAL                         0xFF            // CRC-8 initial value

/* Exported types ------------------------------------------------------------*/

/* AHT21 function status structures definition  */
typedef enum
{
    AHT21_OK                = 0,            /* Operation completed successfully.    */
    AHT21_ERROR             = 1,            /* Run-time error without case matched  */
    AHT21_ERROR_TIMEOUT     = 2,            /* Operation failed with timeout        */
    AHT21_ERROR_RESOURCE    = 3,            /* Resource not available.              */
    AHT21_ERROR_PARAMETER   = 4,            /* Parameter error.                     */
    AHT21_ERROR_NOMEMORY    = 5,            /* Out of memory.                       */
    AHT21_ERROR_ISR         = 6,            /* Not allowed in ISR context           */
    AHT21_RESERVED          = 0x7FFFFFFF    /* Reserved                             */
} aht21_status_t;

typedef enum 
{
    AHT21_INITED = 0,           /* AHT21 initialized        */
    AHT21_NOT_INITED            /* AHT21 not initialized    */
} aht21_inited_t;


#ifdef SOFTWARE_IIC // Implemented using software IIC

/* AHT21 software IIC interface structures definition  */
typedef struct
{
    aht21_status_t (*pf_iic_init)(void);                    /* IIC init interface           */
    aht21_status_t (*pf_iic_deinit)(void);                  /* IIC deinit interface         */
    aht21_status_t (*pf_iic_start)(void);                   /* IIC start signal interface   */
    aht21_status_t (*pf_iic_stop)(void);                    /* IIC stop signal interface    */
    aht21_status_t (*pf_iic_send_byte)(uint8_t);            /* IIC send byte interface      */
    aht21_status_t (*pf_iic_receive_byte)(uint8_t *);       /* IIC receive byte interface   */
    aht21_status_t (*pf_iic_send_ack)(void);                /* IIC wait ack interface       */
    aht21_status_t (*pf_iic_send_no_ack)(void);             /* IIC Init interface           */
    aht21_status_t (*pf_iic_wait_ack)(void);                /* IIC wait ack interface       */

#ifdef OS_SUPPORTING
    aht21_status_t (*pf_critical_enter)(void);              /* IIC enter critical state     */
    aht21_status_t (*pf_critical_exit)(void);               /* IIC exit critical state      */
#endif
} iic_driver_interface_t;

#endif /* SOFTWARE_IIC */

#ifndef SOFTWARE_IIC // Implemented using hardware IIC

/* AHT21 hardware IIC interface structures definition  */
typedef struct
{
    aht21_status_t (*pf_iic_init)(void);         /* IIC init interface           */
    aht21_status_t (*pf_iic_deinit)(void);       /* IIC deinit interface         */
    aht21_status_t (*pf_iic_send_byte)(void);    /* IIC send byte interface      */
    aht21_status_t (*pf_iic_receive_byte)(void); /* IIC receive byte interface   */
    aht21_status_t (*pf_iic_send_ack)(void);     /* IIC wait ack interface       */
    aht21_status_t (*pf_iic_send_no_ack)(void);  /* IIC Init interface           */

} iic_driver_interface_t;

#endif /* HARDWARE_IIC */

/* AHT21 timebase interface structures definition  */
typedef struct
{
    uint32_t (*pf_get_tick_count)(void); /*  Get tick number interface  */
} timebase_interface_t;

/* AHT21 os delay interface structures definition  */
#ifdef OS_SUPPORTING
typedef struct
{
    uint32_t (*pf_rtos_yield)(const uint32_t); /*  Get tick number interface  */
} yield_interface_t;
#endif

/* AHT21 instance class definition */
typedef struct bsp_aht21_driver
{
    // Status flag
    aht21_inited_t  iic_inited_status;    /* IIC initialization status */

    // Function table
    iic_driver_interface_t *p_iic_driver_interface;
    timebase_interface_t *p_timebase_interface;
#ifdef OS_SUPPORTING
    yield_interface_t *p_yield_interface;
#endif

    // AHT21 member methods
    aht21_status_t (*pf_init)(struct bsp_aht21_driver * const );                              /*  AHT21 init              */
    aht21_status_t (*pf_deinit)(struct bsp_aht21_driver * const );                            /*  AHT21 deinit            */
    aht21_status_t (*pf_read_temperature)(struct bsp_aht21_driver * const, float * const);    /*  AHT21 read temperature  */
    aht21_status_t (*pf_read_humidity)(struct bsp_aht21_driver * const, float * const);       /*  AHT21 read humidity     */
    aht21_status_t (*pf_reset)(struct bsp_aht21_driver * const );                             /*  AHT21 reset             */
    aht21_status_t (*pf_read_id)(struct bsp_aht21_driver * const );                           /*  AHT21 read ID           */
    aht21_status_t (*pf_sleep)(struct bsp_aht21_driver * const);                              /*  AHT21 sleep             */
    aht21_status_t (*pf_wakeup)(struct bsp_aht21_driver * const);                             /*  AHT21 wakeup            */
} bsp_aht21_driver_t;

/* Exported functions ------------------------------------------------------------*/

// create AHT21 driver instance
aht21_status_t aht21_create(bsp_aht21_driver_t *const p_aht21_driver,
                            iic_driver_interface_t *const p_iic_driver_interface,
                            timebase_interface_t *const p_timebase_interface,
#ifdef OS_SUPPORTING
                            yield_interface_t *const p_yield_interface
#endif
);

#endif /* __BSP_AHT21_DRIVER_H */
