#include "uart_parse_task.h"

uint8_t frame_buf[MAX_FRAME_LEN];
uint16_t frame_len = 0;

static void process_frame(uint8_t *frame_buf, uint16_t frame_len)
{
    uint8_t checksum_recv;
    uint8_t checksum_calc = 0;
    int i = 0;

    if (frame_len == 0) {
        log_e("Empty frame (no data)");
        return;
    }

    // 校验和为帧数据区的最后一个字节
    checksum_recv = frame_buf[frame_len - 1];
    for (i = 0; i < frame_len - 1; i++) {
        checksum_calc += frame_buf[i];
    }

    if (checksum_calc != checksum_recv) {
        log_e("Checksum error: calc=0x%02X, recv=0x%02X", checksum_calc, checksum_recv);
        return;
    }

    // 校验通过，输出数据
    for (i = 0; i < frame_len - 1; i++) {
        log_i("data: 0x%02X", frame_buf[i]);
    }
    log_i("data end");
    log_i(" ");
}

void uart_backendTask_Function(void *argument)
{
    uint32_t notify = 0;
    uint8_t byte = 0;
    uart_task_status_t task_status = IDLE;
	
	memset(frame_buf, 0, MAX_FRAME_LEN);

    for(;;)
    {
        // 1. 阻塞等待前端通知有数据
        if (xQueueReceive(uart_to_backend_Queue, &notify, portMAX_DELAY) != pdTRUE) {
            log_e("Queue receive error!");
            continue;
        }
        if (notify != SEND_TO_BACKEND)
        {
            log_e("Invalid backend flag!");
            continue;
        }
        
        while (1)
        {
            buffer_status_t status =  circular_buffer_pop(&cb, &byte);
            if (status == BUFFER_EMPTY) {
                break;   // 缓冲区已空，等待下次通知
            }

            // 3. 状态机处理每个字节
            switch (task_status) {
                case IDLE:
                    if (byte == START_OF_FRAME) {
                        log_i("data start");
                        task_status = RECV;
                    }
                    break;
                case RECV:
                    if (byte == END_OF_FRAME) {
                        process_frame(frame_buf, frame_len);
                        frame_len = 0;
                        task_status = IDLE;
                    } else {
					    // 存储数据（包括校验和字节）
						if (frame_len < MAX_FRAME_LEN) {
							frame_buf[frame_len++] = byte;
						} else {
							log_e("Frame too long, discarding");
							frame_len = 0;
							task_status = IDLE;
						}
					}
                    break;              
                default:
                    break;
            }
        }
    }
}


