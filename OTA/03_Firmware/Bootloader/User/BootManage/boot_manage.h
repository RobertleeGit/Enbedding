#ifndef __BOOT_MANAGE_H
#define __BOOT_MANAGE_H

#include <stdint.h>
#include "stm32f4xx.h"
#include "flash.h"
#include "AES.h"

#define APP_START_ADDRESS 0x08008000    /* APP运行起始地址 */
#define APP_BACK_ADDRESS  0x08020000    /* APP备份起始地址 */

typedef void (*pFunction)(void);
typedef enum {
    COPY_SUCCESS = 0,
    COPY_ERROR
} copy_status_t;




void jump_to_app(uint32_t app_address);
copy_status_t copy_back_to_app(uint32_t back_address, uint32_t app_address, int app_size);
copy_status_t decodeCopy_back_to_app(uint32_t back_address, uint32_t app_address, int size);

#endif	/* __BOOT_MANAGE_H */

