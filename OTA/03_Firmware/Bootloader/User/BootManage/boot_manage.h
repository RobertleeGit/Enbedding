#ifndef __BOOT_MANAGE_H
#define __BOOT_MANAGE_H

#include <stdint.h>
#include "stm32f4xx.h"


#define ApplicationAddress			0x08008000
typedef void (*pFunction)(void);


void jump_to_app(void);

#endif	/* __BOOT_MANAGE_H */

