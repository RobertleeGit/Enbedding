/**
 ******************************************************************************
 * @file    ymodem_port.h
 * @brief   Ymodem 协议底层硬件抽象接口 (HAL)
 * @note    本文件定义了 Ymodem 协议层所需的所有底层操作接口。
 *          协议层通过本接口操作串口和存储，无需感知具体硬件细节。
 *
 *          【设计原则】
 *          - 串口接口：非阻塞接收 + 阻塞发送
 *          - 存储接口：基于偏移量 (offset) 操作，协议层不感知绝对地址
 *          - 协议层通过 YmodemPort_StorageInit() 指定写入目标，
 *            后续所有存储操作均相对于该基准偏移
 ******************************************************************************
 */

#ifndef _YMODEM_PORT_H
#define _YMODEM_PORT_H

/* 包含头文件 ----------------------------------------------------------------*/
#include <stdint.h>

/* --- 返回值 --- */
#define YMODEM_PORT_OK      ( 0)   /* 操作成功 */
#define YMODEM_PORT_ERR     (-1)   /* 操作失败 */


#define FLASH_INTERNAL      0   /* 内部 Flash 存储 (STM32 内部 Flash) */
#define FLASH_EXTERNAL      1   /* 外部 Flash 存储 (SPI Flash 等) */

/* ============================================================
 * 串口底层接口
 * ============================================================ */

/**
 * @brief  非阻塞检测串口是否有数据到达
 * @param  key: 输出参数，存储接收到的字节
 * @retval 1: 接收到数据，key 中存放有效字节
 * @retval 0: 暂无数据到达
 * @note   必须是非阻塞实现！Ymodem 协议依赖超时检测，
 *         阻塞会破坏协议握手时序
 */
uint32_t SerialKeyPressed(uint8_t *key);

/**
 * @brief  通过串口发送一个字节
 * @param  c: 要发送的字节
 * @note   阻塞式发送，会等待发送寄存器为空
 */
void SerialPutChar(uint8_t c);

/* ============================================================
 * 存储抽象接口 (Flash / 外部 Flash / EEPROM 等)
 *
 * 使用流程:
 *   1. YmodemPort_StorageInit(base_addr)  — 初始化写入目标
 *   2. YmodemPort_GetCapacity()           — 查询可用容量
 *   3. YmodemPort_StorageErase(size)      — 擦除目标区域
 *   4. YmodemPort_StorageWrite(offset, data, len)  — 按偏移写入
 *
 * 协议层仅使用【字节偏移量】，不感知存储介质的绝对地址，
 * 从而支持内部 Flash、外部 SPI Flash 等不同存储介质。
 * ============================================================ */

/**
 * @brief  初始化存储子系统，设置写入基准
 * @param  base_address: 存储写入的起始地址 (内部 Flash / 外部 Flash 地址)
 * @note   必须在 Ymodem_Receive() 之前调用。
 *         内部会记录基准地址并计算可用容量。
 */
void YmodemPort_StorageInit(uint32_t base_address);

/**
 * @brief  获取从基准地址到存储末尾的可用容量 (字节)
 * @retval 可用字节数
 * @note   调用前必须先执行 YmodemPort_StorageInit()
 */
uint32_t YmodemPort_GetCapacity(void);

/**
 * @brief  擦除从基准地址开始的指定大小区域
 * @param  size: 要擦除的字节数
 * @retval YMODEM_PORT_OK  ( 0): 擦除成功
 * @retval YMODEM_PORT_ERR (-1): 擦除失败
 * @note   内部自动转换为绝对地址调用底层擦除函数
 */
int32_t YmodemPort_StorageErase(uint32_t size);

/**
 * @brief  从基准偏移处写入一块数据，并逐 Word 读回校验
 * @param  offset: 相对于基准地址的字节偏移 (必须 4 字节对齐)
 * @param  data:   数据源指针
 * @param  length: 要写入的字节数 (必须为 4 的倍数)
 * @retval YMODEM_PORT_OK  ( 0): 写入并校验成功
 * @retval YMODEM_PORT_ERR (-1): 写入或校验失败
 * @note   内部逐 Word 写入并立即读回比对，确保数据完整性
 */
int32_t YmodemPort_StorageWrite(uint32_t offset, const uint8_t *data, uint32_t length);

#endif /* _YMODEM_PORT_H */

