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


// 复制备份区 APP 到运行区
copy_status_t copy_back_to_app(uint32_t back_address, uint32_t app_address, int app_size) 
{
    // 检查 app_size 是否超过 APP 区域大小 (0x08008000 ~ 0x08020000)
    if (app_size <= 0 || app_size > (0x18000 - 1))
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

// AES 向量
unsigned char IV[16] = {0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32,
						0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32};
// AES 密钥
unsigned char Key[32] = {0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32,
						 0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32,
						 0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32,
						 0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32};


// 复制并解密备份区 APP( AES256 加密) 到运行区
// 密文格式为 自定义数据长度(4B) + 自定义数据(nB) + 明文长度(4B) + 明文数据(nB) + 填充数据(PB)
// 自定义数据长度 + 8字节 (两个长度信息) 必须为 16 的整数倍
copy_status_t decodeCopy_back_to_app(uint32_t back_address, uint32_t app_address, int size)
{
    uint8_t * pIV_IN_OUT = IV;		// CBC 链式向量：输入 IV，输出上轮密文
	uint8_t * pAES_key256 = Key;	// 密钥
	uint8_t buffer[16] = {0x00};
	uint32_t app_size = 0;

	// size 为文件加密后的大小，会多 16 字节的辅助信息
	if (size <= 16 || size > (0x18010 - 1))
	{
		return COPY_ERROR;
	}

	/* 1. 解密首 16 字节辅助信息块 */
	memcpy(buffer, (uint8_t *)back_address, 16);
	Aes_IV_key256bit_Decode(pIV_IN_OUT, buffer, pAES_key256);
	app_size = buffer[12] | (buffer[13] << 8) | (buffer[14] << 16) | (buffer[15] << 24);

	// 校验自定义长度及解密出的 app_size
	if (app_size == 0 || app_size > (0x18000 - 1))
	{
		return COPY_ERROR;
	}

	/* 2. 擦除要写入 Flash 扇区 */
	if (Erase_Area(app_address, app_size) != FLASH_COMPLETE)
	{
		return COPY_ERROR;
	}

	/* 3. 解密并拷贝到目标地址（CBC 模式逐块解密 + Flash 字写入校验） */
	uint32_t remaining  = app_size;
	uint32_t src_offset = 16;  // 跳过 16 字节辅助信息头

	while (remaining > 0 && src_offset < (uint32_t)size)
	{
		memcpy(buffer, (uint8_t *)(back_address + src_offset), 16);
		Aes_IV_key256bit_Decode(pIV_IN_OUT, buffer, pAES_key256);

		uint32_t *p_word = (uint32_t *)buffer;
		for (int j = 0; j < 4 && remaining >= 4; j++)
		{
			if (Program_Word(app_address, *p_word) != FLASH_COMPLETE)
			{
				return COPY_ERROR;
			}

			/* 读回校验 */
			if (*(__IO uint32_t *)app_address != *p_word)
			{
				return COPY_ERROR;
			}

			app_address += 4;
			p_word++;
			remaining   -= 4;
		}

		src_offset += 16;
	}

	return COPY_SUCCESS;
}

