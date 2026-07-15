#ifndef __CIRCULAR_BUFFER_H
#define __CIRCULAR_BUFFER_H

#include <stdint.h>

typedef enum{
    BUFFER_OK = 0,
    BUFFER_ERROR,
    BUFFER_FULL,
    BUFFER_EMPTY
} buffer_status_t;


// 环形缓冲区结构体
typedef struct {
    uint8_t *buffer;   // 指向外部数组的指针
    uint16_t size;     // 缓冲区容量（元素个数）
    uint16_t read;     // 读指针（指向下一个待读取位置）
    uint16_t write;    // 写指针（指向下一个待写入位置）
} circular_buffer_t;

// 初始化缓冲区
void circular_buffer_init(circular_buffer_t *cb, uint8_t *buf, uint16_t size);

// 判空
buffer_status_t circular_buffer_is_empty(const circular_buffer_t *cb);

// 判满
buffer_status_t circular_buffer_is_full(const circular_buffer_t *cb);

// 插入一个字节
buffer_status_t circular_buffer_push(circular_buffer_t *cb, uint8_t data);

// 获取一个字节
buffer_status_t circular_buffer_pop(circular_buffer_t *cb, uint8_t *data);

#endif /* __CIRCULAR_BUFFER_H */


