/**
 ******************************************************************************
 * @file    w25q64_handler.c
 * @brief   W25Q64 外部 SPI Flash 抽象层实现
 * @note    封装 w25qxx.c 底层驱动，统一映射为 W25Q64_HANDLER_OK/ERR 状态码。
 *          上层代码不直接调用 W25Qx_* 函数，通过本层实现与驱动的解耦。
 ******************************************************************************
 */

#include "w25q64_handler.h"
#include "w25qxx.h"

/* ============================================================
 * API 实现
 * ============================================================ */

/**
 * @brief  初始化 W25Q64 SPI Flash
 */
int32_t W25Q64_Handler_Init(void)
{
    if (W25Qx_Init() == W25Qx_OK)
    {
        return W25Q64_HANDLER_OK;
    }
    return W25Q64_HANDLER_ERR;
}

/**
 * @brief  擦除指定地址范围的 Flash 区域
 * @note   按 64KB Block 为单位逐块擦除。
 *         W25Qx_Erase_Block() 擦除一个 64KB 块 (SECTOR_ERASE_CMD 0x20)。
 */
int32_t W25Q64_Handler_Erase(uint32_t addr, uint32_t size)
{
    uint32_t end_addr = addr + size;
    uint32_t block_addr;

    /* 对齐到 64KB Block 边界向下取整 */
    block_addr = addr & ~(W25Q64_SECTOR_SIZE - 1);

    while (block_addr < end_addr)
    {
        if (W25Qx_Erase_Block(block_addr) != W25Qx_OK)
        {
            return W25Q64_HANDLER_ERR;
        }
        block_addr += W25Q64_SECTOR_SIZE;
    }

    return W25Q64_HANDLER_OK;
}

/**
 * @brief  向 Flash 写入数据
 * @note   直接转发到 W25Qx_Write()，内部已处理页对齐和跨页。
 */
int32_t W25Q64_Handler_Write(uint32_t addr, const uint8_t *data, uint32_t size)
{
    if (W25Qx_Write((uint8_t *)data, addr, size) == W25Qx_OK)
    {
        return W25Q64_HANDLER_OK;
    }
    return W25Q64_HANDLER_ERR;
}

/**
 * @brief  从 Flash 读取数据
 */
int32_t W25Q64_Handler_Read(uint32_t addr, uint8_t *data, uint32_t size)
{
    if (W25Qx_Read(data, addr, size) == W25Qx_OK)
    {
        return W25Q64_HANDLER_OK;
    }
    return W25Q64_HANDLER_ERR;
}

/**
 * @brief  获取 Flash 总容量
 */
uint32_t W25Q64_Handler_GetSize(void)
{
    return W25Q64_FLASH_SIZE;
}


