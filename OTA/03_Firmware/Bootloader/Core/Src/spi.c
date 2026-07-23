#include "spi.h"


extern void TimingDelay_Decrement(void);

/* 初始化 SPI 引脚复用 GPIO */
static void SPI_GPIO_Config(void)
{
    // 初始化 GPIO
    GPIO_InitTypeDef GPIO_InitStructure;
    // 开启时钟
    RCC_AHB1PeriphClockCmd(FLASH_CS_GPIO_CLK | FLASH_SPI_SCK_GPIO_CLK
                           | FLASH_SPI_MISO_GPIO_CLK | FLASH_SPI_MOSI_GPIO_CLK, ENABLE);
    // 初始化结构体
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;        // 复用模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      // 推挽输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;    // 不上拉也不下拉
    // 初始化 SCK 引脚
    GPIO_InitStructure.GPIO_Pin = FLASH_SPI_SCK_PIN;
    GPIO_Init(FLASH_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);
    // 初始化 MISO 引脚
    GPIO_InitStructure.GPIO_Pin = FLASH_SPI_MISO_PIN;
    GPIO_Init(FLASH_SPI_MISO_GPIO_PORT, &GPIO_InitStructure);
    // 初始化 MOSI 引脚
    GPIO_InitStructure.GPIO_Pin = FLASH_SPI_MOSI_PIN;
    GPIO_Init(FLASH_SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);
    // 初始化 CS(NSS) 引脚
    GPIO_InitStructure.GPIO_Pin = FLASH_CS_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_Init(FLASH_CS_GPIO_PORT, &GPIO_InitStructure);

    // 设置引脚复用
    GPIO_PinAFConfig(FLASH_SPI_SCK_GPIO_PORT, FLASH_SPI_SCK_PINSOURCE, FLASH_SPI_SCK_AF); 
	GPIO_PinAFConfig(FLASH_SPI_MISO_GPIO_PORT, FLASH_SPI_MISO_PINSOURCE, FLASH_SPI_MISO_AF); 
	GPIO_PinAFConfig(FLASH_SPI_MOSI_GPIO_PORT, FLASH_SPI_MOSI_PINSOURCE, FLASH_SPI_MOSI_AF); 
}


/* 初始化 SPI */
static void SPI_Config(void)
{
    // 初始化结构体
    SPI_InitTypeDef SPI_InitStructure;

    // 开启时钟
    RCC_APB2PeriphClockCmd(FLASH_SPI_CLK, ENABLE);

    // 配置 SPI 初始化参数
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  // 全双工
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;                       // 主机模式
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;                   // 8位数据帧
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;                         // 时钟空闲状态为高电平
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;                        // 数据在偶数时钟边沿被采样 
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;                           // 软件管理NSS信号
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;  // 波特率预分频值
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;                  // 数据传输从MSB位开始
    SPI_InitStructure.SPI_CRCPolynomial = 7;                            // CRC值计算的多项式
    SPI_Init(FLASH_SPI, &SPI_InitStructure);

    // 使能 SPI
    SPI_Cmd(FLASH_SPI, ENABLE);
}

/**
 * @brief  SPI Flash 初始化函数
 * @param  None
 * @return None
 */
void SPI_Flash_Init(void)
{
    SPI_GPIO_Config();
    SPI_Config();
}


/**
 * @brief  SPI 发送一个字节并接收一个字节
 * @param  transmitByte: 要发送的字节
 * @return 接收到的数据
 */
static uint8_t SPI_FLASH_TransceiveByte(uint8_t transmitByte)
{
    // 等待发送缓冲区空
    while (SPI_I2S_GetFlagStatus(FLASH_SPI,SPI_I2S_FLAG_TXE) == RESET);
    // 发送数据
    SPI_I2S_SendData(FLASH_SPI, transmitByte);
    
    // 等待接收缓冲区非空
    while (SPI_I2S_GetFlagStatus(FLASH_SPI,SPI_I2S_FLAG_RXNE) == RESET);

    // 必须执行接受函数以清除 RXNE 标志，否则 SPI 将无法继续通信
    return SPI_I2S_ReceiveData(FLASH_SPI);
}

/**
  * @brief  Transmit an amount of data in blocking mode.
  * @param  pData pointer to data buffer
  * @param  Size amount of data bytes to be sent
  * @param  Timeout Timeout duration in ms (not used in blocking mode)
  * @retval SPI_FLASH status
  */
spi_flash_status_t SPI_FLASH_Transmit(uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    uint32_t tickstart = GetTick();

    while (Size--)
    {
        SPI_FLASH_TransceiveByte(*pData++);

        /* 超时检查 */
        if ((GetTick() - tickstart) > Timeout)
        {
            return SPI_FLASH_TIMEOUT;
        }
    }
    return SPI_FLASH_OK;
}


/**
  * @brief  Receive an amount of data in blocking mode.
  * @param  pData pointer to data buffer
  * @param  Size amount of data bytes to be received
  * @param  Timeout Timeout duration in ms (not used in blocking mode)
  * @retval SPI_FLASH status
  */
spi_flash_status_t SPI_FLASH_Receive(uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    uint32_t tickstart = GetTick();

    while (Size--)
    {
        *pData++ = SPI_FLASH_TransceiveByte(0xFF);  /* 发送 dummy 字节读取数据 */

        /* 超时检查 */
        if ((GetTick() - tickstart) > Timeout)
        {
            return SPI_FLASH_TIMEOUT;
        }
    }
    return SPI_FLASH_OK;
}





/**
  * @brief  SysTick interrupt handler.
  *         
  * @retval None
  */



