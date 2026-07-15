#include "circular_buffer.h"

void circular_buffer_init(circular_buffer_t *cb, uint8_t buf[], uint16_t size)
{
    cb->buffer = buf;
    cb->size   = size;
    cb->read   = 0;
    cb->write  = 0;
}

buffer_status_t circular_buffer_is_empty(const circular_buffer_t *cb)
{
    if (cb->read == cb->write)
    {
        return BUFFER_EMPTY;
    }
    return BUFFER_OK;
}

buffer_status_t circular_buffer_is_full(const circular_buffer_t *cb)
{
    // 预留一个空位以区分空满
    if ((cb->write + 1) % cb->size == cb->read)
    {
        return BUFFER_FULL;
    }

    return BUFFER_OK;
}

buffer_status_t circular_buffer_push(circular_buffer_t *cb, uint8_t data)
{
    if (circular_buffer_is_full(cb)) {
        return BUFFER_FULL;
    }
    cb->buffer[cb->write] = data;
    cb->write = (cb->write + 1) % cb->size;
    return BUFFER_OK;
}

buffer_status_t circular_buffer_pop(circular_buffer_t *cb, uint8_t *data)
{
    if (circular_buffer_is_empty(cb)) {
        return BUFFER_EMPTY;
    }
    *data = cb->buffer[cb->read];
    cb->read = (cb->read + 1) % cb->size;
    return BUFFER_OK;
}
