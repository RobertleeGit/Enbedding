#include "flash.h"


static uint32_t Get_Sector_Num(uint32_t addr);


static void Flash_Unlock(void) {
    FLASH_Unlock();
    // 若 flash 忙则等待
    while (FLASH_GetStatus() == FLASH_BUSY);
}

static void Flash_Lock(void) {
    FLASH_Lock();
}


FLASH_Status Erase_Sector(uint32_t FLASH_Sector) {
    Flash_Unlock();
    FLASH_Status status = FLASH_EraseSector(FLASH_Sector, VoltageRange_3);
    Flash_Lock();
    return status;
}

FLASH_Status Program_Word(uint32_t address, uint32_t data) {
    Flash_Unlock();
    FLASH_Status status = FLASH_ProgramWord(address, data);
    Flash_Lock();
    return status;
}

uint32_t Get_Flash_Size(void) {
    return 0x80000;  /* 单位: 字节 */
}

/**
 * @brief  擦除从 startAddr 开始、长度为 size 的 Flash 区域
 * @param  startAddr: 擦除起始地址
 * @param  size:      要擦除的总字节数
 * @retval FLASH_COMPLETE: 擦除成功
 * @retval 其他值:          擦除失败 (详见 FLASH_Status 枚举)
 *
 * @note   擦除是按 Sector 进行的，函数会自动计算需要擦除哪些 Sector。
 *         例如：startAddr=0x0800C000, size=96KB
 *         会擦除 Sector 3(16KB) + Sector 4(64KB) + Sector 5 的前 16KB。
 *         实际上 Sector 5 会被完整擦除 (128KB)，因为最小擦除单元是 Sector。
 */
FLASH_Status Erase_Area(uint32_t startAddr, uint32_t size)
{
    uint32_t endAddr = startAddr + size;        /* 擦除结束地址 */
    uint32_t currentSector = Get_Sector_Num(startAddr);  /* 起始 Sector */
    uint32_t lastSector = Get_Sector_Num(endAddr);       /* 结束 Sector */
    FLASH_Status status = FLASH_COMPLETE;

    /* 逐 Sector 擦除，从起始到结束 */
    for (uint32_t sector = currentSector;
         sector <= lastSector && status == FLASH_COMPLETE;
         sector += 8)  /* STM32F4 中每个 Sector 编号间隔 8 */
    {
        status = Erase_Sector(sector);
    }

    return status;
}


/**
 * @brief  将 Flash 地址映射为 STM32F411CE 的 Sector 编号
 * @param  addr: Flash 绝对地址 (必须是 0x08000000 ~ 0x0807FFFF)
 * @retval STM32 标准外设库定义的 FLASH_Sector_n 值
 */
static uint32_t Get_Sector_Num(uint32_t addr)
{
    if (addr < 0x08004000)      return FLASH_Sector_0;   /* 16KB */
    else if (addr < 0x08008000) return FLASH_Sector_1;   /* 16KB */
    else if (addr < 0x0800C000) return FLASH_Sector_2;   /* 16KB */
    else if (addr < 0x08010000) return FLASH_Sector_3;   /* 16KB */
    else if (addr < 0x08020000) return FLASH_Sector_4;   /* 64KB */
    else if (addr < 0x08040000) return FLASH_Sector_5;   /* 128KB */
    else if (addr < 0x08060000) return FLASH_Sector_6;   /* 128KB */
    else                        return FLASH_Sector_7;   /* 128KB */
}


