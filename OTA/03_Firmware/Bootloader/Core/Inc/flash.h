#ifndef __FLASH_H
#define __FLASH_H

#include "stm32f4xx.h"

/**********************************************************
 * @note   STM32F411CE 的 Sector 布局 (512KB):
 *         Sector 0: 0x0800_0000 ~ 0x0800_3FFF  ( 16KB)
 *         Sector 1: 0x0800_4000 ~ 0x0800_7FFF  ( 16KB)
 *         Sector 2: 0x0800_8000 ~ 0x0800_BFFF  ( 16KB)
 *         Sector 3: 0x0800_C000 ~ 0x0800_FFFF  ( 16KB)
 *         Sector 4: 0x0801_0000 ~ 0x0801_FFFF  ( 64KB)
 *         Sector 5: 0x0802_0000 ~ 0x0803_FFFF  (128KB)
 *         Sector 6: 0x0804_0000 ~ 0x0805_FFFF  (128KB)
 *         Sector 7: 0x0806_0000 ~ 0x0807_FFFF  (128KB)
 **********************************************************/

#define FLASH_SIZE 0x80000  /* STM32F411CE 内部 Flash 总大小: 512KB */

uint32_t Get_Flash_Size(void);

FLASH_Status Erase_Sector(uint32_t FLASH_Sector);

FLASH_Status Program_Word(uint32_t address, uint32_t data);

/**
 * @brief  擦除从 startAddr 开始、长度为 size 的 Flash 区域
 * @param  startAddr: 擦除起始地址
 * @param  size:      要擦除的总字节数
 * @retval FLASH_Status: 擦除结果状态
 */
FLASH_Status Erase_Area(uint32_t startAddr, uint32_t size);

#endif


