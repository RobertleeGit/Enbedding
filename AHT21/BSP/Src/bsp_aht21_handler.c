/******************************************************************************
 *
 * @file bsp_aht21_handler.c
 *
 * @par bsp_aht21_handler.h
 *
 * @brief Provide the HAL APIs of AHT21 and corresponding opetions.
 *
 * @version V1.0.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "bsp_aht21_handler.h"


/* Defines -------------------------------------------------------------------*/
#define HANDLER_DEBUG
#define HANDLER_QUEUE_LEN   10
#define OS_MAX_DELAY        0xFFFF

#define check_param(expr)                                   \
    do {                                                    \
        if ((expr)) {                                       \
            return AHT21_HANDLER_ERROR_PARAMETER;           \
        }                                                   \
    } while(0U)

#define IS_VAILD(PARAM)     (PARAM == NULL)

/* Private variables ---------------------------------------------------------*/



/* Private functions prototypes ----------------------------------------------*/

static aht21_handler_status_t aht21_handler_init (aht21_handler_t * handler_instance);

/* Exported function ----------------------------------------------------------*/

/**
 * @brief  Creates an AHT21 handler instance.
 * @param  aht21_instance: Pointer to the AHT21 handler instance
 * @param  function_table: Pointer to the function table
 * @retval aht21_handler_status_t
 */
aht21_handler_status_t AHT21_Handler_Create(aht21_handler_t * handler_instance, handler_function_table_t * inst_argument)
{
    // check param
    check_param(IS_VAILD(handler_instance));
    check_param(IS_VAILD(inst_argument));
    check_param(IS_VAILD(inst_argument->p_iic_driver_interface));
    check_param(IS_VAILD(inst_argument->p_os_interface));
    check_param(IS_VAILD(inst_argument->p_timebase_interface));
    check_param(IS_VAILD(inst_argument->p_yield_interface));

    // init function table
    handler_instance->p_iic_driver_interface = inst_argument->p_iic_driver_interface;
    handler_instance->p_os_interface = inst_argument ->p_os_interface;
    handler_instance->p_timebase_interface = inst_argument->p_timebase_interface;
    handler_instance->p_yield_interface = inst_argument->p_yield_interface;
    
    // init member method
    handler_instance->pf_init = aht21_handler_init;

    // AHT21 handler init
    if (handler_instance->pf_init(handler_instance) != AHT21_HANDLER_OK)
    {
#ifdef HANDLER_DEBUG
        log_e("handler create failed! Init handler failed!");
#endif    
        return AHT21_HANDLER_ERROR_RESOURCE;
    }
    
    return AHT21_HANDLER_OK;
}

/**
 * @brief  Reads temperature and/or humidity from the AHT21 sensor according to the event.
 *         If the elapsed time since the last read of a given data type has not exceeded
 *         the event's lifetime, the cached value is returned without triggering a new IIC
 *         transaction. Otherwise a real sensor read is performed and the cache is updated.
 * 
 * @param  handler_instance: Pointer to the AHT21 handler instance (holds driver, caches, ticks)
 * @param  event: Pointer to the event specifying data type, lifetime, and output buffer pointers
 * 
 * @retval aht21_handler_status_t: AHT21_HANDLER_OK on success, or error code on failure
 */
aht21_handler_status_t Read_Temp_humi(aht21_handler_t * handler_instance, aht21_handler_event_t *event)
{
    aht21_status_t ret = AHT21_OK;
    uint32_t current_tick;
    uint32_t elapsed;

    // Check parameters
    check_param(IS_VAILD(handler_instance));
    check_param(IS_VAILD(event));
    check_param(IS_VAILD(handler_instance->driver_instance));

    // Get the current system tick
    current_tick = handler_instance->p_timebase_interface->pf_get_tick_count();

    // Process temperature reading if requested
    if ((event->type == TEMPERATURE) || (event->type == BOTH))
    {
        check_param(IS_VAILD(event->temperature));

        // Calculate elapsed time since last temperature read (handles uint32_t wrap-around)
        elapsed = (uint32_t)(current_tick - handler_instance->last_temp_tick);

        if (elapsed > event->lifetime)
        {
            // Lifetime expired — perform a real sensor read
            ret = handler_instance->driver_instance->pf_read_temperature(
                        handler_instance->driver_instance, event->temperature);
            if (AHT21_OK != ret)
            {
#ifdef HANDLER_DEBUG
                log_e("Read temperature failed! error code: %d", ret);
#endif
                return AHT21_HANDLER_ERROR;
            }
            // Update cache
            handler_instance->last_temperature = *(event->temperature);
            handler_instance->last_temp_tick = current_tick;
        }
        else
        {
            // Lifetime not exceeded — return cached value
            *(event->temperature) = handler_instance->last_temperature;
        }
    }

    // Process humidity reading if requested
    if ((event->type == HUMIDITY) || (event->type == BOTH))
    {
        check_param(IS_VAILD(event->humidity));

        // Calculate elapsed time since last humidity read (handles uint32_t wrap-around)
        elapsed = (uint32_t)(current_tick - handler_instance->last_humi_tick);

        if (elapsed > event->lifetime)
        {
            // Lifetime expired — perform a real sensor read
            ret = handler_instance->driver_instance->pf_read_humidity(
                        handler_instance->driver_instance, event->humidity);
            if (AHT21_OK != ret)
            {
#ifdef HANDLER_DEBUG
                log_e("Read humidity failed! error code: %d", ret);
#endif
                return AHT21_HANDLER_ERROR;
            }
            // Update cache
            handler_instance->last_humidity = *(event->humidity);
            handler_instance->last_humi_tick = current_tick;
        }
        else
        {
            // Lifetime not exceeded — return cached value
            *(event->humidity) = handler_instance->last_humidity;
        }
    }

    return AHT21_HANDLER_OK;
}


/**
 * @brief  FreeRTOS task entry for the AHT21 handler. Creates the handler and
 *         driver instances internally (their lifetimes match the thread), then
 *         enters an infinite loop: wait for events on the queue, dispatch to
 *         Read_Temp_humi(), and invoke the per-event callback to notify callers.
 * @param[in] argument: Pointer to an aht21_task_init_t bundling the function
 *                      table (input resources) and client (output queue handle).
 * @return None (FreeRTOS task — must not return)
 */
void AHT21_Handler_Task(void * argument)
{
    aht21_task_init_t      *task_init;
    handler_function_table_t *func_table;
    aht21_handler_client_t   *client;
    aht21_handler_t handler;
    bsp_aht21_driver_t driver;
    aht21_handler_event_t event;
    aht21_handler_status_t ret;
    float temp_buf = 0.0f;
    float humi_buf = 0.0f;

    // Retrieve the task-init struct passed by the caller
    task_init  = (aht21_task_init_t *)argument;
    func_table = task_init->p_func_table;
    client     = task_init->p_client;

    // Zero-initialise locally allocated instances
    memset(&handler, 0, sizeof(handler));
    memset(&driver,  0, sizeof(driver));

    // Bind the driver instance into the handler
    handler.driver_instance = &driver;

    // Create the handler (allocates event queue, creates AHT21 driver, etc.)
    ret = AHT21_Handler_Create(&handler, func_table);
    if (AHT21_HANDLER_OK != ret)
    {
#ifdef HANDLER_DEBUG
        log_e("AHT21 handler task: Handler_Create failed, ret: %d", ret);
#endif
        // Signal to external senders that initialisation failed
        if (client != NULL)
        {
            client->event_queue_handle = NULL;
        }
        return;   /* Task returns → FreeRTOS deletes the task */
    }

    // Publish the queue handle via the client struct so external senders
    // can post events without touching the function table.
    if (client != NULL)
    {
        client->event_queue_handle = handler.event_queue_handle;
    }

    // Bind event output buffers to local storage
    event.temperature = &temp_buf;
    event.humidity    = &humi_buf;

    // Main task loop — never exits
    for (;;)
    {
        // Block until an event arrives from an external producer
        ret = handler.p_os_interface->os_queue_receive(
                    (void **)&(handler.event_queue_handle),
                    (void *)&event,
                    (uint32_t)OS_MAX_DELAY);

        if (AHT21_HANDLER_OK != ret)
        {
#ifdef HANDLER_DEBUG
            log_w("AHT21 handler task: queue receive failed, ret: %d", ret);
#endif
            continue;
        }

        // Dispatch the event to the read routine (with lifetime caching)
        ret = Read_Temp_humi(&handler, &event);
        if (AHT21_HANDLER_OK != ret)
        {
#ifdef HANDLER_DEBUG
            log_e("AHT21 handler task: Read_Temp_humi failed, ret: %d", ret);
#endif
        }

        // Notify the event producer via its callback
        if (event.pf_callback != NULL)
        {
            event.pf_callback(event.temperature, event.humidity);
        }
    }
}




/* Private function ----------------------------------------------------------*/

/**
 * @brief  Initialises the AHT21 handler instance by creating the event queue,
 *         instantiating the AHT21 driver, and setting the handler's init status.
 * @param[in] handler_instance: Pointer to the handler instance to initialise
 * @retval aht21_handler_status_t: AHT21_HANDLER_OK on success, or error code
 */
static aht21_handler_status_t aht21_handler_init (aht21_handler_t * handler_instance)
{
    aht21_handler_status_t ret = AHT21_HANDLER_OK;

    // check param
    check_param(IS_VAILD(handler_instance));
    check_param(IS_VAILD(handler_instance->p_os_interface));
    check_param(IS_VAILD(handler_instance->p_iic_driver_interface));
    check_param(IS_VAILD(handler_instance->p_timebase_interface));
    check_param(IS_VAILD(handler_instance->p_yield_interface));

    // create the queue handler
    ret = handler_instance->p_os_interface->os_queue_create((void **)&(handler_instance->event_queue_handle), 
                                                            (uint32_t)HANDLER_QUEUE_LEN, 
                                                            (uint32_t)sizeof(aht21_handler_event_t));
    if (AHT21_HANDLER_OK != ret)    
    {
#ifdef HANDLER_DEBUG
        log_e("handler init failed! Can't create event queue!");
#endif
        return AHT21_HANDLER_ERROR_RESOURCE;
    }

    // 以下代码原本也应和 handler 代码解耦
    // create the driver instance
    ret = aht21_create(handler_instance->driver_instance, 
                        handler_instance->p_iic_driver_interface,
                        handler_instance->p_timebase_interface,
                        handler_instance->p_yield_interface);
    
    if (AHT21_OK != ret)    
    {
#ifdef HANDLER_DEBUG
        log_e("handler init failed! Can't create driver instance!");
#endif
        return AHT21_HANDLER_ERROR_RESOURCE;
    }

    // update the init status
    handler_instance->init_status = AHT21_HANDLER_INIT;
    
    return AHT21_HANDLER_OK;

}


