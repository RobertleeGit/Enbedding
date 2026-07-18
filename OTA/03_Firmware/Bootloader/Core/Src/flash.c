#include "flash.h"


void Flash_Unlock(void) {
    FLASH_Unlock();
    // 若 flash 忙则等待
    while (FLASH_GetStatus() == FLASH_BUSY);
}

void Flash_Lock(void) {
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


