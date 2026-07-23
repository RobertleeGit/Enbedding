#ifndef __W25Q64_HANDLER_H
#define __W25Q64_HANDLER_H

/**
 ******************************************************************************
 * @file    w25q64_handler.h
 * @brief   W25Q64 外部 SPI Flash 抽象接口
 * @note    封装 w25qxx.c 底层驱动，向上层提供统一的 Flash 操作 API。
 *          上层代码 (如 ymodem_port) 只依赖此头文件，无需直接包含 w25qxx.h，
 *          从而实现与 SPI Flash 底层驱动的解耦。
 *
 *         W25Q64 规格: 64 Mbit = 8 MByte
 *         - 128 个 64KB 的 Block (Sector)
 *         - 2048 个 4KB 的 SubSector
 *         - 32768 个 256 字节的 Page
 ******************************************************************************
 */

#include <stdint.h>

/* ============================================================
 * 宏定义
 * ============================================================ */

#define W25Q64_HANDLER_OK      ( 0)   /* 操作成功 */
#define W25Q64_HANDLER_ERR     (-1)   /* 操作失败 */

#define W25Q64_FLASH_SIZE      (0x800000)  /* W25Q64 总容量: 8 MB */
#define W25Q64_SECTOR_SIZE     (0x10000)   /* 64KB Block 大小 */

/* ============================================================
 * API 函数声明
 * ============================================================ */

/**
 * @brief  初始化 W25Q64 SPI Flash
 * @retval W25Q64_HANDLER_OK  ( 0): 初始化成功
 * @retval W25Q64_HANDLER_ERR (-1): 初始化失败 (芯片 ID 不匹配)
 */
int32_t W25Q64_Handler_Init(void);

/**
 * @brief  擦除指定地址范围的 Flash 区域
 * @param  addr: 擦除起始地址 (芯片内部偏移，0 ~ 8MB-1)
 * @param  size: 要擦除的字节数
 * @retval W25Q64_HANDLER_OK  ( 0): 擦除成功
 * @retval W25Q64_HANDLER_ERR (-1): 擦除失败
 * @note   按 64KB Block 为单位逐块擦除，不满一块也会擦除整块。
 */
int32_t W25Q64_Handler_Erase(uint32_t addr, uint32_t size);

/**
 * @brief  向 Flash 写入数据
 * @param  addr: 写入起始地址 (芯片内部偏移，0 ~ 8MB-1)
 * @param  data: 数据源指针
 * @param  size: 要写入的字节数
 * @retval W25Q64_HANDLER_OK  ( 0): 写入成功
 * @retval W25Q64_HANDLER_ERR (-1): 写入失败
 * @note   内部自动处理页边界对齐 (256 字节/页)。
 *         写入前需确保目标区域已擦除 (全 0xFF)。
 */
int32_t W25Q64_Handler_Write(uint32_t addr, const uint8_t *data, uint32_t size);

/**
 * @brief  从 Flash 读取数据
 * @param  addr: 读取起始地址 (芯片内部偏移，0 ~ 8MB-1)
 * @param  data: 数据目标缓冲区
 * @param  size: 要读取的字节数
 * @retval W25Q64_HANDLER_OK  ( 0): 读取成功
 * @retval W25Q64_HANDLER_ERR (-1): 读取失败
 */
int32_t W25Q64_Handler_Read(uint32_t addr, uint8_t *data, uint32_t size);

/**
 * @brief  获取 Flash 总容量
 * @retval Flash 总字节数 (8 MB = 0x800000)
 */
uint32_t W25Q64_Handler_GetSize(void);









#endif /* __W25Q64_HANDLER_H */

