/**
 ******************************************************************************
 * @file    ymodem_port.c
 * @brief   Ymodem 协议底层硬件接口实现
 * @note    提供串口收发 和 Flash 操作的硬件抽象层
 ******************************************************************************
 */

/* 包含头文件 ----------------------------------------------------------------*/
#include "ymodem_port.h"

/* ============================================================
 * 串口底层实现
 * ============================================================ */

/**
 * @brief  非阻塞检测 USART1 是否有数据到达
 * @param  key: 输出参数，存储接收到的字节
 * @retval 1: 接收到数据
 * @retval 0: 暂无数据
 *
 * @note   【关键】此函数必须是非阻塞的！
 *         Ymodem 协议通过轮询 + 超时来实现握手机制。
 *         如果改为阻塞等待，协议的超时逻辑将无法工作，
 *         导致升级流程卡死。
 *
 */
uint32_t SerialKeyPressed(uint8_t *key)
{
    /* 检查 USART1 接收寄存器是否非空 (RXNE = Rx Not Empty) */
    if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET)
    {
        /* 读取接收到的字节，同时自动清除 RXNE 标志 */
        *key = (uint8_t)USART_ReceiveData(USART1);
        return 1;
    }
    return 0;
}

/**
 * @brief  通过 USART1 阻塞发送一个字节
 * @param  c: 要发送的字节
 *
 * @note   会等待发送数据寄存器 (TDR) 为空后再写入。
 *         对于 Bootloader 中低频率的 Ymodem 应答包 (ACK/NAK/CRC16)，
 *         阻塞发送完全可接受。
 */
void SerialPutChar(uint8_t c)
{
    /* 等待发送寄存器为空 (TXE = Tx Empty) */
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    /* 写入要发送的字节 */
    USART_SendData(USART1, c);
}

/**
 * @brief  擦除指定 Flash 区域
 */
int32_t YmodemPort_FlashErase(uint32_t startAddr, uint32_t size)
{
    if (Erase_Area(startAddr, size) == FLASH_COMPLETE)
    {
        return YMODEM_PORT_OK;
    }
    return YMODEM_PORT_ERR;
}

/**
 * @brief  向 Flash 写入一个 Word (4 字节)
 */
int32_t YmodemPort_FlashWriteWord(uint32_t address, uint32_t data)
{
    if (Program_Word(address, data) == FLASH_COMPLETE)
    {
        return YMODEM_PORT_OK;
    }
    return YMODEM_PORT_ERR;
}

/**
 * @brief  校验 Flash 写入结果 (读回比对)
 */
int32_t YmodemPort_FlashVerify(uint32_t address, uint32_t expected)
{
    if (*(uint32_t *)address == expected)
    {
        return YMODEM_PORT_OK;
    }
    return YMODEM_PORT_ERR;
}


/**
 * @brief  获取 Flash 总大小 (字节)
 */
uint32_t YmodemPort_GetFlashSize(void)
{
    return Get_Flash_Size();
}


