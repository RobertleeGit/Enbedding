/**
 ******************************************************************************
 * @file    ymodem_port.h
 * @brief   Ymodem 协议通用接口 — 串口底层收发 & Flash 地址配置
 * @note    本文件仅保留 Ymodem 接收所需的最小接口
 ******************************************************************************
 */

#ifndef _YMODEM_PORT_H
#define _YMODEM_PORT_H

/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "usart.h"
#include "flash.h"



#define YMODEM_PORT_OK      ( 0)   /* 操作成功 */
#define YMODEM_PORT_ERR     (-1)   /* 操作失败 */

/* ============================================================
 * 串口底层接口
 * ============================================================ */

/**
 * @brief  非阻塞检测 USART1 是否有数据到达
 * @param  key: 输出参数，存储接收到的字节
 * @retval 1: 接收到数据，key 中存放有效字节
 * @retval 0: 暂无数据到达
 * @note   必须是非阻塞实现！Ymodem 协议依赖超时检测，
 *         阻塞会破坏协议握手时序
 */
uint32_t SerialKeyPressed(uint8_t *key);

/**
 * @brief  通过 USART1 发送一个字节
 * @param  c: 要发送的字节
 * @note   阻塞式发送，会等待发送寄存器为空
 */
void SerialPutChar(uint8_t c);

/* ============================================================
 * Flash 底层抽象接口
 * ============================================================ */

/**
 * @brief  擦除指定 Flash 区域
 * @param  startAddr: 擦除起始地址
 * @param  size:      要擦除的字节数
 * @retval YMODEM_PORT_OK  ( 0): 擦除成功
 * @retval YMODEM_PORT_ERR (-1): 擦除失败
 * @note   封装底层 Erase_Area()，逐 Sector 擦除。
 *         Ymodem 协议层通过此接口操作 Flash，无需感知 STM32 标准外设库。
 */
int32_t YmodemPort_FlashErase(uint32_t startAddr, uint32_t size);

/**
 * @brief  向 Flash 写入一个 Word (4 字节)
 * @param  address: 写入目标地址 (必须 4 字节对齐)
 * @param  data:    要写入的 32 位数据
 * @retval YMODEM_PORT_OK  ( 0): 写入成功
 * @retval YMODEM_PORT_ERR (-1): 写入失败
 * @note   封装底层 Program_Word()。STM32F4 要求写入前必须先擦除。
 */
int32_t YmodemPort_FlashWriteWord(uint32_t address, uint32_t data);

/**
 * @brief  校验 Flash 写入结果 (读回比对)
 * @param  address:  要校验的 Flash 地址
 * @param  expected: 期望的 32 位数值
 * @retval YMODEM_PORT_OK  ( 0): 数据一致
 * @retval YMODEM_PORT_ERR (-1): 数据不一致
 * @note   直接从目标地址读回并逐位比对，确保写入正确。
 */
int32_t YmodemPort_FlashVerify(uint32_t address, uint32_t expected);


/**
 * @brief  获取 Flash 总大小 (字节)
 */
uint32_t YmodemPort_GetFlashSize(void);

#endif /* _YMODEM_PORT_H */

