#include "boot_manage.h"

/**
  * @brief  从 Bootloader 跳转到 APP 应用程序
  */
void jump_to_app(uint32_t app_address)
{
    uint32_t JumpAddress;
    pFunction Jump_To_Application;

    /* 检查栈顶地址是否合法 */
    if (((*(__IO uint32_t *)app_address) & 0x2FFE0000) == 0x20000000)
    {
        /* 屏蔽所有中断，防止在跳转过程中，中断干扰出现异常 */
        __disable_irq();
        NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x8000);
        RCC_DeInit();
        /* 用户代码区第二个 字 为程序开始地址(复位地址) */
        JumpAddress = *(__IO uint32_t *) (app_address + 4);

        /* Initialize user application's Stack Pointer */
        /* 初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址) */
        __set_MSP(*(__IO uint32_t *) app_address);

        /* 类型转换 */
        Jump_To_Application = (pFunction) JumpAddress;

        /* 跳转到 APP */
        Jump_To_Application();
    }
}


copy_status_t copy_back_to_app(uint32_t back_address, uint32_t app_address, uint32_t app_size) 
{
    // 检查 app_size 是否超过 APP 区域大小 (0x08008000 ~ 0x08020000)
    if (app_size > (0x18000 - 1))
    {
        return COPY_ERROR;
    }

    // 擦除需要写入的 Flash 扇区
    if (Erase_Area(app_address, app_size) != FLASH_COMPLETE)
    {
        return COPY_ERROR;
    }

    uint32_t *src = (uint32_t *)back_address;
    uint32_t *dst = (uint32_t *)app_address;

    for (uint32_t i = 0; i < app_size; i += 4)
    {
		uint32_t data = *src;
        if (Program_Word((uint32_t)dst, data) != FLASH_COMPLETE)
        {
            return COPY_ERROR;
        }

        /* 读回校验 */
        if (*dst != *src)
        {
            return COPY_ERROR;
        }

        src++;
        dst++;
    }
    return COPY_SUCCESS;
}

