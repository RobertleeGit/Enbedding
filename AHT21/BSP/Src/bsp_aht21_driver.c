/******************************************************************************
 *
 * @file bsp_aht21_driver.c
 *
 * @par bsp_aht21_driver.h
 *
 * @brief Provide the HAL APIs of AHT21 and corresponding opetions.
 *
 * @version V1.0.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "bsp_aht21_driver.h"
#include "elog.h"

/* Defines -------------------------------------------------------------------*/
#define DEBUG                                   // Open debug code

#define check_param(expr)                           \
    do {                                            \
        if ((expr)) {                               \
            return AHT21_ERROR_PARAMETER;           \
        }                                           \
    } while(0U)

#define IS_VAILD(PARAM)     (PARAM == NULL)
#define IS_IIC_INITED(INSTANCE) ((INSTANCE)->iic_inited_status != AHT21_INITED)
#define IIC_CONTEXT(AHT21_INSTANCE)    AHT21_INSTANCE->p_iic_driver_interface->context

/* Private variables ---------------------------------------------------------*/



/* Private functions prototypes ----------------------------------------------*/

static uint8_t Check_Crc8                      (const uint8_t *p_data, const uint8_t len);
static aht21_status_t aht21_read_raw_data      (bsp_aht21_driver_t * const aht21_instance, uint8_t *raw_data);
static void aht21_delay_ms                     (bsp_aht21_driver_t * const aht21_instance, uint32_t ms);


static aht21_status_t aht21_init               (bsp_aht21_driver_t * const aht21_instance );
static aht21_status_t aht21_deinit             (bsp_aht21_driver_t * const aht21_instance );
static aht21_status_t aht21_read_temperature   (bsp_aht21_driver_t * const aht21_instance, float * const temp);
static aht21_status_t aht21_read_humidity      (bsp_aht21_driver_t * const aht21_instance, float * const humi);
static aht21_status_t aht21_reset              (bsp_aht21_driver_t * const aht21_instance );
static aht21_status_t aht21_read_id            (bsp_aht21_driver_t * const aht21_instance );
static aht21_status_t aht21_sleep              (bsp_aht21_driver_t * const aht21_instance);
static aht21_status_t aht21_wakeup             (bsp_aht21_driver_t * const aht21_instance);



/* Exported function ----------------------------------------------------------*/


/** 
 * @brief  Construct an AHT21 driver instantiation object
 * 
 * @param[out] p_aht21_driver: Pointer to the bsp_aht21_driver_t
 * @param[in] p_iic_driver_interface: Pointer to the iic_driver_interface_t
 * @param[in] p_timebase_interface: Pointer to the timebase_interface_t
 * @param[in] p_yield_interface: Pointer to the yield_interface_t (need OS support)
 * 
 * @return aht21_status_t
 */
aht21_status_t aht21_create(bsp_aht21_driver_t *const p_aht21_driver,
                            iic_driver_interface_t *const p_iic_driver_interface,
                            timebase_interface_t *const p_timebase_interface,
#ifdef OS_SUPPORTING
                            yield_interface_t *const p_yield_interface
#endif
)
{
    // check param
    check_param(IS_VAILD(p_aht21_driver));
    check_param(IS_VAILD(p_iic_driver_interface));
    check_param(IS_VAILD(p_timebase_interface));
#ifdef OS_SUPPORTING
    check_param(IS_VAILD(p_yield_interface));
#endif

    // init function table
    p_aht21_driver->p_iic_driver_interface = p_iic_driver_interface;
    p_aht21_driver->p_timebase_interface = p_timebase_interface;
#ifdef OS_SUPPORTING
    p_aht21_driver->p_yield_interface = p_yield_interface;
#endif

    // init member methods
    p_aht21_driver->pf_init = aht21_init;
    p_aht21_driver->pf_deinit = aht21_deinit;
    p_aht21_driver->pf_read_temperature = aht21_read_temperature;
    p_aht21_driver->pf_read_humidity = aht21_read_humidity;
    p_aht21_driver->pf_reset = aht21_reset;
    p_aht21_driver->pf_read_id = aht21_read_id;
    p_aht21_driver->pf_sleep = aht21_sleep;
    p_aht21_driver->pf_wakeup = aht21_wakeup;

    return AHT21_OK;
}


/* Private function ----------------------------------------------------------*/

/** 
 * @brief  Calculate CRC-8 checksum for the given data buffer.
 * 
 * @param[in] p_data: Pointer to the data buffer to be checked
 * @param[in] len: Length of the data buffer in bytes
 * 
 * @return Calculated 8-bit CRC value
 */
static uint8_t Check_Crc8(const uint8_t *p_data, const uint8_t len)
{
    uint8_t crc = CRC8_INTIAL;      // crc value variable
    
    for (uint8_t idx = 0; idx < len; idx++) {
        crc ^= p_data[idx];
        // process each bit of the current byte
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 0x80) {       // If MSB is 1
                crc = (crc << 1) ^ CRC8_POLYNOMIAL;     
            } else {
                crc <<= 1;
            }
        }
    }
    
    return crc;
}

/** 
 * @brief Read the AHT21 status byte 
 * 
 * @param[in] aht21_instance: Const pointer to the bsp_aht21_driver_t
 * @param[out] status_byte: Pointer to the receive status byte variable
 * 
 * @note steps:
 *          1. Send read address (0x71) to IIC bus
 *          2. Receive one byte
 * 
 * @return aht21_status_t
 */
aht21_status_t aht21_read_status(bsp_aht21_driver_t * const aht21_instance, uint8_t *status_byte)
{
    // check param
    check_param(IS_VAILD(aht21_instance));
    check_param(IS_VAILD(aht21_instance->p_iic_driver_interface));
    check_param(IS_VAILD(status_byte));
    check_param(IS_IIC_INITED(aht21_instance));


    aht21_status_t ret = AHT21_OK;
#ifdef SOFTWARE_IIC
    aht21_instance->p_iic_driver_interface->
                    pf_critical_enter(IIC_CONTEXT(aht21_instance));      // Enter critical
#endif
    // Send IIC start signal
    aht21_instance->p_iic_driver_interface->
                    pf_iic_start(IIC_CONTEXT(aht21_instance));
    // Send read address to read status byte
    aht21_instance->p_iic_driver_interface->
                    pf_iic_send_byte(IIC_CONTEXT(aht21_instance), AHT21_READ_ADD);  
    // Waiting ack 
    ret = aht21_instance->p_iic_driver_interface->
                    pf_iic_wait_ack(IIC_CONTEXT(aht21_instance));
    if (AHT21_OK != ret)
    {
        return ret;
    }
    // Receive status byte
    aht21_instance->p_iic_driver_interface->
                    pf_iic_receive_byte(IIC_CONTEXT(aht21_instance), status_byte);

    // Send no ack signal from master to slave
    aht21_instance->p_iic_driver_interface->
                    pf_iic_send_no_ack(IIC_CONTEXT(aht21_instance));
    
    // Send the stop signal
    aht21_instance->p_iic_driver_interface->
                    pf_iic_stop(IIC_CONTEXT(aht21_instance));

#ifdef SOFTWARE_IIC
    aht21_instance->p_iic_driver_interface->
                    pf_critical_exit(IIC_CONTEXT(aht21_instance));      // Exit critical
#endif

#ifdef DEBUG
    log_d("Aht21 status byte = 0x%2x", *status_byte);
#endif

    return ret;
}


/** 
 *  @brief  Blocking delay in milliseconds (using timebase tick).
 */
static void aht21_delay_ms (bsp_aht21_driver_t * const aht21_instance, uint32_t ms)
{
    uint32_t start = aht21_instance->p_timebase_interface->pf_get_tick_count();
    while ((aht21_instance->p_timebase_interface->pf_get_tick_count() - start) < ms)
    {
#ifdef OS_SUPPORTING
        aht21_instance->p_yield_interface->pf_rtos_yield(1);
#endif
    }
}

/** 
 *  @brief  Trigger measurement and read raw 6-byte data from AHT21.
 *          Raw data layout: [Hum_H, Hum_L, Hum_mid, Temp_H, Temp_L, Temp_mid]
 *          where temp_mid byte also contains CRC data.
 *          Per the manual, reads: Status (1B) + Data (6B: hum[2.5B]|temp[2.5B] + CRC[1B])
 * 
 *  @param[in]  aht21_instance: AHT21 driver instance
 *  @param[out] raw_data: Buffer for 6 raw bytes (status + data excluding CRC)
 * 
 *  @note Steps:
 *          1. Send trigger cmd 0xAC with params 0x33, 0x00
 *          2. Wait 80ms for measurement to complete
 *          3. Send I2C read address (0x71)
 *          4. Read status byte, then 6 data bytes, then CRC byte
 *          5. Verify CRC
 */
static aht21_status_t aht21_read_raw_data (bsp_aht21_driver_t * const aht21_instance, uint8_t *raw_data)
{
    aht21_status_t ret = AHT21_OK;
    uint8_t buf[8] = {0};  // status + 6 data + 1 CRC = 8 bytes read
    uint8_t crc_calc;

    check_param(IS_VAILD(aht21_instance));
    check_param(IS_VAILD(aht21_instance->p_iic_driver_interface));
    check_param(IS_VAILD(raw_data));
    check_param(IS_IIC_INITED(aht21_instance));

#ifdef SOFTWARE_IIC
    aht21_instance->p_iic_driver_interface->
                    pf_critical_enter(IIC_CONTEXT(aht21_instance));
#endif

    /* --- 1. Send trigger measurement command: 0xAC + 0x33 + 0x00 --- */
    aht21_instance->p_iic_driver_interface->
                    pf_iic_start(IIC_CONTEXT(aht21_instance));
    aht21_instance->p_iic_driver_interface->
                    pf_iic_send_byte(IIC_CONTEXT(aht21_instance), AHT21_WRITE_ADD);
    ret = aht21_instance->p_iic_driver_interface->
                    pf_iic_wait_ack(IIC_CONTEXT(aht21_instance));
    if (AHT21_OK != ret) { goto exit_err; }

    aht21_instance->p_iic_driver_interface->
                    pf_iic_send_byte(IIC_CONTEXT(aht21_instance), AHT21_CMD_TRIGGER_MEASUREMENT);
    ret = aht21_instance->p_iic_driver_interface->
                    pf_iic_wait_ack(IIC_CONTEXT(aht21_instance));
    if (AHT21_OK != ret) { goto exit_err; }

    aht21_instance->p_iic_driver_interface->
                    pf_iic_send_byte(IIC_CONTEXT(aht21_instance), 0x33);
    ret = aht21_instance->p_iic_driver_interface->
                    pf_iic_wait_ack(IIC_CONTEXT(aht21_instance));
    if (AHT21_OK != ret) { goto exit_err; }

    aht21_instance->p_iic_driver_interface->
                    pf_iic_send_byte(IIC_CONTEXT(aht21_instance), 0x00);
    ret = aht21_instance->p_iic_driver_interface->
                    pf_iic_wait_ack(IIC_CONTEXT(aht21_instance));
    if (AHT21_OK != ret) { goto exit_err; }

    aht21_instance->p_iic_driver_interface->
                    pf_iic_stop(IIC_CONTEXT(aht21_instance));

#ifdef SOFTWARE_IIC
    aht21_instance->p_iic_driver_interface->
                    pf_critical_exit(IIC_CONTEXT(aht21_instance));
#endif

    /* --- 2. Wait 80ms for measurement to complete --- */
    aht21_delay_ms(aht21_instance, AHT21_MEASURE_READY_TIME);

#ifdef SOFTWARE_IIC
    aht21_instance->p_iic_driver_interface->
                    pf_critical_enter(IIC_CONTEXT(aht21_instance));
#endif

    /* --- 3 & 4. Read measurement results --- */
    aht21_instance->p_iic_driver_interface->
                    pf_iic_start(IIC_CONTEXT(aht21_instance));
    aht21_instance->p_iic_driver_interface->
                    pf_iic_send_byte(IIC_CONTEXT(aht21_instance), AHT21_READ_ADD);
    ret = aht21_instance->p_iic_driver_interface->
                    pf_iic_wait_ack(IIC_CONTEXT(aht21_instance));
    if (AHT21_OK != ret) { goto exit_err; }

    // Read status byte + 6 data bytes (ACK each), then 1 CRC byte (NAK)
    for (uint8_t i = 0; i < 7; i++)
    {
        ret = aht21_instance->p_iic_driver_interface->
                    pf_iic_receive_byte(IIC_CONTEXT(aht21_instance), &buf[i]);
        if (AHT21_OK != ret) { goto exit_err; }
        
        if (i < 6)
        {
            aht21_instance->p_iic_driver_interface->
                            pf_iic_send_ack(IIC_CONTEXT(aht21_instance));
        }
        else
        {
            aht21_instance->p_iic_driver_interface->
                            pf_iic_send_no_ack(IIC_CONTEXT(aht21_instance));
        }
    }

    aht21_instance->p_iic_driver_interface->
                    pf_iic_stop(IIC_CONTEXT(aht21_instance));

#ifdef SOFTWARE_IIC
    aht21_instance->p_iic_driver_interface->
                    pf_critical_exit(IIC_CONTEXT(aht21_instance));
#endif

    /* --- 5. Verify CRC (over status + first 6 data bytes) --- */
    crc_calc = Check_Crc8(buf, 7);
    if (crc_calc != buf[7])
    {
#ifdef DEBUG
        log_d("AHT21 CRC error: calc=0x%02X, recv=0x%02X", crc_calc, buf[7]);
#endif
        return AHT21_ERROR;
    }

    /* --- Check status: bit[7] must be 0 (not busy) --- */
    if (buf[0] & 0x80)
    {
#ifdef DEBUG
        log_d("AHT21 busy, status=0x%02X", buf[0]);
#endif
        return AHT21_ERROR_TIMEOUT;
    }

    // Copy 5 data bytes (excluding status and CRC)
    for (uint8_t i = 0; i < 5; i++)
    {
        raw_data[i] = buf[i + 1];
    }

    return AHT21_OK;

exit_err:
    aht21_instance->p_iic_driver_interface->
                    pf_iic_stop(IIC_CONTEXT(aht21_instance));
#ifdef SOFTWARE_IIC
    aht21_instance->p_iic_driver_interface->
                    pf_critical_exit(IIC_CONTEXT(aht21_instance));
#endif
    return ret;
}

/** 
 * @brief Initialize the AHT21 (hardware)
 * 
 * @param[in] aht21_instance: Const pointer to the bsp_aht21_driver_t
 * 
 * @note steps (per product manual section 5.4):
 *          1. Power-up wait 40ms (SCL=HIGH) for sensor to reach idle state
 *          2. Read status byte via 0x71. Check Bit[3] (CAL Enable)
 *          3. If Bit[3] != 1: Send init cmd 0xBE with params 0x08, 0x00
 *          4. Wait 10ms for initialization to complete
 * 
 * @return aht21_status_t
 */
static aht21_status_t aht21_init (bsp_aht21_driver_t * const aht21_instance )
{
    aht21_status_t ret = AHT21_OK;
    uint8_t status_byte = 0;

    check_param(IS_VAILD(aht21_instance));
    check_param(IS_VAILD(aht21_instance->p_iic_driver_interface));
    check_param(IS_VAILD(aht21_instance->p_timebase_interface));

#ifdef DEBUG
    log_d("AHT21 init start");
#endif
	
    /* 1. Init IIC */
    aht21_instance->p_iic_driver_interface->
                    pf_iic_init(IIC_CONTEXT(aht21_instance));
    /* 2. Power-up delay: 40ms for sensor to be ready */
    aht21_delay_ms(aht21_instance, 40);

    /* 3. Read status byte and check calibration bit[3] */
    ret = aht21_read_status(aht21_instance, &status_byte);
    if (AHT21_OK != ret)
    {
#ifdef DEBUG
        log_d("AHT21 read status failed");
#endif
        return ret;
    }
    // Bit[3] CAL Enable = 0 -> not calibrated, then init aht21
    if (!(status_byte & 0x08))  
    {
#ifdef DEBUG
        log_d("AHT21 not calibrated, sending init cmd");
#endif

#ifdef SOFTWARE_IIC
        aht21_instance->p_iic_driver_interface->
                        pf_critical_enter(IIC_CONTEXT(aht21_instance));
#endif

        /* Send init command 0xBE with params 0x08, 0x00 */
        aht21_instance->p_iic_driver_interface->
                        pf_iic_start(IIC_CONTEXT(aht21_instance));
        aht21_instance->p_iic_driver_interface->
                        pf_iic_send_byte(IIC_CONTEXT(aht21_instance), AHT21_WRITE_ADD);
        ret = aht21_instance->p_iic_driver_interface->
                        pf_iic_wait_ack(IIC_CONTEXT(aht21_instance));
        if (AHT21_OK != ret) { goto init_exit; }

        aht21_instance->p_iic_driver_interface->
                        pf_iic_send_byte(IIC_CONTEXT(aht21_instance), AHT21_CMD_INIT);
        ret = aht21_instance->p_iic_driver_interface->
                        pf_iic_wait_ack(IIC_CONTEXT(aht21_instance));
        if (AHT21_OK != ret) { goto init_exit; }

        aht21_instance->p_iic_driver_interface->
                        pf_iic_send_byte(IIC_CONTEXT(aht21_instance), 0x08);
        ret = aht21_instance->p_iic_driver_interface->
                        pf_iic_wait_ack(IIC_CONTEXT(aht21_instance));
        if (AHT21_OK != ret) { goto init_exit; }

        aht21_instance->p_iic_driver_interface->
                        pf_iic_send_byte(IIC_CONTEXT(aht21_instance), 0x00);
        ret = aht21_instance->p_iic_driver_interface->
                        pf_iic_wait_ack(IIC_CONTEXT(aht21_instance));
        if (AHT21_OK != ret) { goto init_exit; }

        aht21_instance->p_iic_driver_interface->
                        pf_iic_stop(IIC_CONTEXT(aht21_instance));

#ifdef SOFTWARE_IIC
        aht21_instance->p_iic_driver_interface->
                        pf_critical_exit(IIC_CONTEXT(aht21_instance));
#endif

    /* 4. Wait 10ms for initialization to complete */
        aht21_delay_ms(aht21_instance, 10);
    }

    /* Mark IIC as initialized */
    aht21_instance->iic_inited_status = AHT21_INITED;

#ifdef DEBUG
    log_d("AHT21 init end");
#endif

    return AHT21_OK;

init_exit:
    aht21_instance->p_iic_driver_interface->
                    pf_iic_stop(IIC_CONTEXT(aht21_instance));
#ifdef SOFTWARE_IIC
    aht21_instance->p_iic_driver_interface->
                    pf_critical_exit(IIC_CONTEXT(aht21_instance));
#endif
    return ret;
}

/** 
 * @brief Deinit the AHT21 
 * 
 * @param[in] aht21_instance: Const pointer to the bsp_aht21_driver_t
 * 
 * @note Deinitialize the IIC interface.
 * 
 * @return aht21_status_t
 */
static aht21_status_t aht21_deinit (bsp_aht21_driver_t * const aht21_instance )
{
    check_param(IS_VAILD(aht21_instance));
    check_param(IS_VAILD(aht21_instance->p_iic_driver_interface));

    aht21_instance->p_iic_driver_interface->
                    pf_iic_deinit(IIC_CONTEXT(aht21_instance));
    aht21_instance->iic_inited_status = AHT21_NOT_INITED;

    return AHT21_OK;
}

/** 
 * @brief Reading the temperature from AHT21
 * 
 * @param[in]  aht21_instance: AHT21 driver instance
 * @param[out] temp: Output temperature in degrees Celsius
 * 
 * @note ST = raw temperature 20-bit value
 *       T[℃] = (ST / 2^20) * 200 - 50
 * 
 * @return aht21_status_t
 */
static aht21_status_t aht21_read_temperature (bsp_aht21_driver_t * const aht21_instance, float * const temp)
{
    aht21_status_t ret;
    uint8_t raw_data[6] = {0};
    uint32_t st;

    check_param(IS_VAILD(aht21_instance));
    check_param(IS_VAILD(temp));
    check_param(IS_IIC_INITED(aht21_instance));

    ret = aht21_read_raw_data(aht21_instance, raw_data);
    if (AHT21_OK != ret)
    {
        return ret;
    }

    /* Extract 20-bit temperature value:
       raw_data[2] & 0x0F: Temp[19:16] (lower nibble of combined byte)
       raw_data[3]: Temp[15:8]
       raw_data[4]: Temp[7:0] */
    st = ((uint32_t)(raw_data[2] & 0x0F) << 16) |
         ((uint32_t)raw_data[3] << 8) |
         ((uint32_t)raw_data[4]);

    /* T[℃] = (ST / 2^20) * 200 - 50 */
    *temp = ((float)st * 200.0f) / 1048576.0f - 50.0f;

#ifdef DEBUG
    log_d("AHT21 temperature: %.2f (raw=0x%06lX)", *temp, st);
#endif

    return AHT21_OK;
}

/** 
 * @brief Reading the humidity from AHT21
 * 
 * @param[in]  aht21_instance: AHT21 driver instance
 * @param[out] humi: Output relative humidity in %RH
 * 
 * @note Per manual section 6.1:
 *          SRH = raw humidity 20-bit value
 *          RH[%] = (SRH / 2^20) * 100%
 * 
 * @return aht21_status_t
 */
static aht21_status_t aht21_read_humidity (bsp_aht21_driver_t * const aht21_instance, float * const humi)
{
    aht21_status_t ret;
    uint8_t raw_data[6] = {0};
    uint32_t srh;

    check_param(IS_VAILD(aht21_instance));
    check_param(IS_VAILD(humi));
    check_param(IS_IIC_INITED(aht21_instance));

    ret = aht21_read_raw_data(aht21_instance, raw_data);
    if (AHT21_OK != ret)
    {
        return ret;
    }

    /* Extract 20-bit humidity value:
       raw_data[0]: Hum[19:12]
       raw_data[1]: Hum[11:4]
       raw_data[2]: Hum[3:0]  (upper nibble) */
    srh = ((uint32_t)raw_data[0] << 12) |
          ((uint32_t)raw_data[1] << 4) |
          ((uint32_t)raw_data[2] >> 4);

    /* RH[%] = (SRH / 2^20) * 100 */
    *humi = ((float)srh * 100.0f) / 1048576.0f;

#ifdef DEBUG
    log_d("AHT21 humidity: %.2f %%RH (raw=0x%06lX)", *humi, srh);
#endif

    return AHT21_OK;
}


/** 
 * @brief Software reset AHT21.
 * 
 * @param[in] aht21_instance: AHT21 driver instance
 * 
 * @note Sends 0xBA command. Sensor re-initializes within 20ms.
 * 
 * @return aht21_status_t
 */
static aht21_status_t aht21_reset (bsp_aht21_driver_t * const aht21_instance )
{
    aht21_status_t ret = AHT21_OK;

    check_param(IS_VAILD(aht21_instance));
    check_param(IS_VAILD(aht21_instance->p_iic_driver_interface));
    check_param(IS_IIC_INITED(aht21_instance));

#ifdef SOFTWARE_IIC
    aht21_instance->p_iic_driver_interface->
                    pf_critical_enter(IIC_CONTEXT(aht21_instance));
#endif

    aht21_instance->p_iic_driver_interface->
                    pf_iic_start(IIC_CONTEXT(aht21_instance));
    aht21_instance->p_iic_driver_interface->
                    pf_iic_send_byte(IIC_CONTEXT(aht21_instance), AHT21_WRITE_ADD);
    ret = aht21_instance->p_iic_driver_interface->
                          pf_iic_wait_ack(IIC_CONTEXT(aht21_instance));
    if (AHT21_OK != ret) { goto reset_exit; }

    aht21_instance->p_iic_driver_interface->
                    pf_iic_send_byte(IIC_CONTEXT(aht21_instance), AHT21_CMD_RESET);
    ret = aht21_instance->p_iic_driver_interface->
                          pf_iic_wait_ack(IIC_CONTEXT(aht21_instance));
    if (AHT21_OK != ret) { goto reset_exit; }

    aht21_instance->p_iic_driver_interface->
                    pf_iic_stop(IIC_CONTEXT(aht21_instance));

#ifdef SOFTWARE_IIC
    aht21_instance->p_iic_driver_interface->
                    pf_critical_exit(IIC_CONTEXT(aht21_instance));
#endif

    /* Wait 20ms for soft reset to complete */
    aht21_delay_ms(aht21_instance, 20);

#ifdef DEBUG
    log_d("AHT21 soft reset done");
#endif

    return AHT21_OK;

reset_exit:
    aht21_instance->p_iic_driver_interface->
                    pf_iic_stop(IIC_CONTEXT(aht21_instance));
#ifdef SOFTWARE_IIC
    aht21_instance->p_iic_driver_interface->
                    pf_critical_exit(IIC_CONTEXT(aht21_instance));
#endif
    return ret;
}

/** 
 * @brief Read AHT21 status byte and verify device presence.
 * 
 * @param[in] aht21_instance: AHT21 driver instance
 * 
 * @return AHT21_OK if status byte is successfully read.
 */
static aht21_status_t aht21_read_id (bsp_aht21_driver_t * const aht21_instance)
{
    uint8_t status = 0;

    check_param(IS_VAILD(aht21_instance));

    return aht21_read_status(aht21_instance, &status);
}

/** 
 * @brief Put AHT21 into sleep mode.
 * 
 * @param[in] aht21_instance: AHT21 driver instance
 * 
 * @note AHT21 enters idle/sleep state automatically after measurement
 *       completes. According to the product manual, sending a trigger
 *       measurement command and then stopping will leave the sensor
 *       in measurement mode. To enter sleep, we ensure no pending
 *       measurement is running.
 * 
 * @return aht21_status_t
 */
static aht21_status_t aht21_sleep (bsp_aht21_driver_t * const aht21_instance)
{
    check_param(IS_VAILD(aht21_instance));

    /* AHT21 enters idle state automatically after measurement.
       Just verify communication is alive by reading status. */
    uint8_t status = 0;
    (void)aht21_read_status(aht21_instance, &status);

    return AHT21_OK;
}

/** 
 * @brief Wake up AHT21 from sleep mode.
 * 
 * @param[in] aht21_instance: AHT21 driver instance
 * 
 * @note Per the product manual, sending a trigger measurement command
 *       (0xAC) wakes the sensor and starts a new measurement cycle.
 *       After wake-up, the sensor needs 80ms to complete measurement.
 * 
 * @return aht21_status_t
 */
static aht21_status_t aht21_wakeup (bsp_aht21_driver_t * const aht21_instance)
{
    aht21_status_t ret = AHT21_OK;

    check_param(IS_VAILD(aht21_instance));
    check_param(IS_VAILD(aht21_instance->p_iic_driver_interface));

#ifdef SOFTWARE_IIC
    aht21_instance->p_iic_driver_interface->
                    pf_critical_enter(IIC_CONTEXT(aht21_instance));
#endif

    /* Send trigger measurement to wake up sensor */
    aht21_instance->p_iic_driver_interface->
                    pf_iic_start(IIC_CONTEXT(aht21_instance));
    aht21_instance->p_iic_driver_interface->
                    pf_iic_send_byte(IIC_CONTEXT(aht21_instance), AHT21_WRITE_ADD);
    ret = aht21_instance->p_iic_driver_interface->
                    pf_iic_wait_ack(IIC_CONTEXT(aht21_instance));
    if (AHT21_OK != ret) { goto wakeup_exit; }

    aht21_instance->p_iic_driver_interface->
                    pf_iic_send_byte(IIC_CONTEXT(aht21_instance), AHT21_CMD_TRIGGER_MEASUREMENT);
    ret = aht21_instance->p_iic_driver_interface->
                    pf_iic_wait_ack(IIC_CONTEXT(aht21_instance));
    if (AHT21_OK != ret) { goto wakeup_exit; }

    aht21_instance->p_iic_driver_interface->
                    pf_iic_send_byte(IIC_CONTEXT(aht21_instance), 0x33);
    ret = aht21_instance->p_iic_driver_interface->
                    pf_iic_wait_ack(IIC_CONTEXT(aht21_instance));
    if (AHT21_OK != ret) { goto wakeup_exit; }

    aht21_instance->p_iic_driver_interface->
                    pf_iic_send_byte(IIC_CONTEXT(aht21_instance), 0x00);
    ret = aht21_instance->p_iic_driver_interface->
                    pf_iic_wait_ack(IIC_CONTEXT(aht21_instance));
    if (AHT21_OK != ret) { goto wakeup_exit; }

    aht21_instance->p_iic_driver_interface->
                    pf_iic_stop(IIC_CONTEXT(aht21_instance));

#ifdef SOFTWARE_IIC
    aht21_instance->p_iic_driver_interface->
                    pf_critical_exit(IIC_CONTEXT(aht21_instance));
#endif

    /* Wait for measurement to complete */
    aht21_delay_ms(aht21_instance, AHT21_MEASURE_READY_TIME);

    return AHT21_OK;

wakeup_exit:
    aht21_instance->p_iic_driver_interface->
                    pf_iic_stop(IIC_CONTEXT(aht21_instance));
#ifdef SOFTWARE_IIC
    aht21_instance->p_iic_driver_interface->
                    pf_critical_exit(IIC_CONTEXT(aht21_instance));
#endif
    return ret;
}

