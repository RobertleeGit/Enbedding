/********************************************************************************************
 *
 * @file bsp_aht21_handler.h
 *
 * @par stdint.h
 * 		stdio.h
 *      bsp_aht21_driver.h
 *
 * @brief Provide the HAL APIs of AHT21 and corresponding opetions.
 *
 * @version V1.0.0
 *
 * @note 1 tab == 4 spaces!
 *
 ********************************************************************************************/

#ifndef __BSP_AHT21_HANDLER_H
#define __BSP_AHT21_HANDLER_H

/* Includes --------------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include "bsp_aht21_driver.h"
#include "elog.h"

/* Defines ---------------------------------------------------------------------------------*/


/* Exported types --------------------------------------------------------------------------*/

typedef enum
{
    AHT21_HANDLER_OK                = 0,            /* Operation completed successfully.    */
    AHT21_HANDLER_ERROR             = 1,            /* Run-time error without case matched  */
    AHT21_HANDLER_ERROR_TIMEOUT     = 2,            /* Operation failed with timeout        */
    AHT21_HANDLER_ERROR_RESOURCE    = 3,            /* Resource not available.              */
    AHT21_HANDLER_ERROR_PARAMETER   = 4,            /* Parameter error.                     */
    AHT21_HANDLER_ERROR_NOMEMORY    = 5,            /* Out of memory.                       */
    AHT21_HANDLER_ERROR_ISR         = 6,            /* Not allowed in ISR context           */
    AHT21_HANDLER_RESERVED          = 0x7FFFFFFF    /* Reserved                             */
} aht21_handler_status_t;

/* Required data type */
typedef enum
{
    TEMPERATURE = 0,
    HUMIDITY,
    BOTH
} aht21_data_type_t;

/* Handler init type */
typedef enum
{
    AHT21_HANDLER_INIT = 0,
    AHT21_HANDLER_NOT_INIT
} aht21_handler_init_status_t;


/* Class of event */
typedef struct 
{
    float *temperature;                         /* Pointer to the temperature receive buffer    */
    float *humidity;                            /* Pointer to the humidity receive buffer       */                     
    uint32_t lifetime;                          /* Lifetime of the data                         */
    uint32_t timestamp;                         /* Timestamp of the event                       */
    aht21_data_type_t type;                     /* The data type                                */
    void (* pf_callback) (float *, float *);    /* The callback function                        */
} aht21_handler_event_t;

/* OS interface structures definition */
typedef struct 
{
    void (*os_delay_ms) (uint32_t ms);           /* OS delay interface */
    aht21_handler_status_t (*os_queue_create) (void ** const queue_handle,
                                               uint32_t queue_length,
                                               uint32_t item_size);
    aht21_handler_status_t (*os_queue_send) (void ** const queue_handle,
                                             void * const item,
                                             uint32_t timeout);
    aht21_handler_status_t (*os_queue_receive) (void ** const queue_handle,
                                                void * const item,
                                                uint32_t timeout);
} os_interface_t;


/* Class of AHT21 handler */
typedef struct aht21_handler
{
    // Member variable
    aht21_handler_init_status_t init_status;            /* Init status of aht21 handler             */
    uint32_t last_temp_tick;                            /* Timestamp of last temperature reading    */
    uint32_t last_humi_tick;                            /* Timestamp of last humidity reading       */
    float last_temperature;                             /* Last read temperature value (cached)     */
    float last_humidity;                                /* Last read humidity value (cached)        */
    
    void * event_queue_handle;                          /* Handle of event queue                    */
    bsp_aht21_driver_t *driver_instance;                /* Instance of aht21 driver                 */
    
    // Function table
    os_interface_t *p_os_interface;                     /* OS function interface                    */
    // Create the AHT21 driver instance
    iic_driver_interface_t *p_iic_driver_interface;     /* IIC dirver interface                     */
    timebase_interface_t *p_timebase_interface;         /* Timebase interface                       */
    yield_interface_t *p_yield_interface;               /* Yield interface                          */

    /* Driver create function pointer */
    aht21_handler_status_t (*pf_init) (struct aht21_handler * aht21_handler_instance);    
    
} aht21_handler_t;

// Encapsulate the function resources needed (input to handler)
typedef struct 
{
    // Function table
    os_interface_t *p_os_interface;
    // Create the AHT21 driver instance
    iic_driver_interface_t *p_iic_driver_interface;
    timebase_interface_t *p_timebase_interface;
    yield_interface_t *p_yield_interface;
} handler_function_table_t;


/* Client handle used by external threads to communicate with the handler.
   Filled by AHT21_Handler_Task after the internal event queue is created.
   External callers use this struct together with the OS interface to send
   events into the handler's event queue.                                   */
typedef struct
{
    void * event_queue_handle;          /* Handle of the handler's event queue */
} aht21_handler_client_t;


/* Task initialisation structure — bundles the resources the handler needs
   (function table) with the output channel (client) that external threads
   will use.  Passed as the argument to AHT21_Handler_Task().               */
typedef struct
{
    handler_function_table_t *p_func_table;   /* Input: interfaces & resources  */
    aht21_handler_client_t   *p_client;       /* Output: queue handle for senders */
} aht21_task_init_t;


/* Exported functions --------------------------------------------------------------------------*/

/**
 * @brief  Creates an AHT21 handler instance.
 * @param  aht21_instance: Pointer to the AHT21 handler instance
 * @param  function_table: Pointer to the function table
 * @retval aht21_handler_status_t
 */
aht21_handler_status_t AHT21_Handler_Create(aht21_handler_t * aht21_instance, handler_function_table_t * inst_argument);

/**
 * @brief  Reads the temperature and humidity from the AHT21 sensor according to the event.
 *         Only triggers a real IIC read when the time elapsed since last reading exceeds
 *         the specified lifetime; otherwise returns the cached value from the last read.
 * @param  handler_instance: Pointer to the AHT21 handler instance
 * @param  event: Pointer to the event containing read request parameters and output buffers
 * @retval aht21_handler_status_t
 */
aht21_handler_status_t Read_Temp_humi(aht21_handler_t * handler_instance, aht21_handler_event_t *event);


/**
 * @brief  FreeRTOS task entry for the AHT21 handler. Creates the handler and
 *         driver instances internally (their lifetimes match the thread), then
 *         enters an infinite loop: wait for events on the queue, read temp/humi
 *         via Read_Temp_humi(), and invoke the event's callback.
 * @param[in] argument: Pointer to an aht21_task_init_t that bundles the
 *                      handler_function_table_t (input resources) and the
 *                      aht21_handler_client_t (output — filled with the queue
 *                      handle after initialisation).
 * @return None (FreeRTOS task — must not return)
 */
void AHT21_Handler_Task(void * argument);



#endif /* __BSP_AHT21_HANDLER_H */
