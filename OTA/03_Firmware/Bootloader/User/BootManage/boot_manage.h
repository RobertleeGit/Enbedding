#ifndef __BOOT_MANAGE_H
#define __BOOT_MANAGE_H

#include <stdint.h>
#include "stm32f4xx.h"
#include "flash.h"
#include "AES.h"

#define APP_START_ADDRESS     0x08008000    /* APP运行起始地址 (内部 Flash) */
#define APP_BACK_ADDRESS      0x08020000    /* APP备份起始地址 (内部 Flash 备份区) */
#define EXT_FLASH_BACK_OFFSET 0x00000000    /* APP备份起始偏移 (外部 W25Q64 Flash) */

typedef void (*pFunction)(void);
typedef enum {
    COPY_SUCCESS = 0,
    COPY_ERROR
} copy_status_t;




void jump_to_app(uint32_t app_address);
copy_status_t copy_back_to_app(uint32_t back_address, uint32_t app_address, int app_size);
copy_status_t decodeCopy_back_to_app(uint32_t back_address, uint32_t app_address, int size);

/**
 * @brief  从外部 Flash 解密并拷贝固件到内部 Flash APP 区
 * @param  ext_offset:  外部 Flash 中密文数据的起始偏移 (0 ~ 8MB-1)
 * @param  app_address: 内部 Flash APP 区起始地址
 * @param  size:        密文数据总大小 (字节, 含 16 字节辅助信息头)
 * @retval COPY_SUCCESS / COPY_ERROR
 * @note   与 decodeCopy_back_to_app 逻辑相同，但源数据从外部 SPI Flash (W25Q64)
 *         读取，而非内部 Flash 直接地址访问。
 */
copy_status_t decodeCopy_ext_to_app(uint32_t ext_offset, uint32_t app_address, int size);

#endif	/* __BOOT_MANAGE_H */

