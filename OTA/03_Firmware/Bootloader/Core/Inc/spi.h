#ifndef __SPI_H
#define __SPI_H

#include "stm32f4xx.h"
#include "main.h"
#include <stdio.h>

/* SPI GPIO 定义 */
// CS(NSS) 引脚
#define FLASH_CS_PIN                        GPIO_Pin_4            
#define FLASH_CS_GPIO_PORT                  GPIOA                     
#define FLASH_CS_GPIO_CLK                   RCC_AHB1Periph_GPIOA
// SCK 引脚
#define FLASH_SPI_SCK_PIN                   GPIO_Pin_5               
#define FLASH_SPI_SCK_GPIO_PORT             GPIOA                       
#define FLASH_SPI_SCK_GPIO_CLK              RCC_AHB1Periph_GPIOA
#define FLASH_SPI_SCK_PINSOURCE             GPIO_PinSource5
#define FLASH_SPI_SCK_AF                    GPIO_AF_SPI1
// MISO 引脚
#define FLASH_SPI_MISO_PIN                  GPIO_Pin_6                
#define FLASH_SPI_MISO_GPIO_PORT            GPIOA                   
#define FLASH_SPI_MISO_GPIO_CLK             RCC_AHB1Periph_GPIOA
#define FLASH_SPI_MISO_PINSOURCE            GPIO_PinSource6
#define FLASH_SPI_MISO_AF                   GPIO_AF_SPI1
// MOSI 引脚
#define FLASH_SPI_MOSI_PIN                  GPIO_Pin_7                
#define FLASH_SPI_MOSI_GPIO_PORT            GPIOA                     
#define FLASH_SPI_MOSI_GPIO_CLK             RCC_AHB1Periph_GPIOA
#define FLASH_SPI_MOSI_PINSOURCE            GPIO_PinSource7
#define FLASH_SPI_MOSI_AF                   GPIO_AF_SPI1


/* SPI 定义  */
#define FLASH_SPI                           SPI1
#define FLASH_SPI_CLK                       RCC_APB2Periph_SPI1

// SPI 状态枚举类型
typedef enum {
    SPI_FLASH_OK = 0,
    SPI_FLASH_ERROR,
    SPI_FLASH_BUSY,
    SPI_FLASH_TIMEOUT
} spi_flash_status_t;

/* 全局 tick 计数器 (需在 SysTick_Handler 中递增) */
extern __IO uint32_t uwTick;

/* 函数声明 */
void SPI_Flash_Init(void);
uint8_t SPI_FLASH_TransceiveByte(uint8_t transmitByte);
spi_flash_status_t SPI_FLASH_Transmit(uint8_t *pData, uint16_t Size, uint32_t Timeout);
spi_flash_status_t SPI_FLASH_Receive(uint8_t *pData, uint16_t Size, uint32_t Timeout);











#endif /* __SPI_H */

