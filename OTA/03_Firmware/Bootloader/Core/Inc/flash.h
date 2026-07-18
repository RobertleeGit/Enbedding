#ifndef __FLASH_H
#define __FLASH_H

#include "stm32f4xx.h"


FLASH_Status Erase_Sector(uint32_t FLASH_Sector);
FLASH_Status Program_Word(uint32_t address, uint32_t data);


#endif


