/**
 ******************************************************************************
 * @file    ymodem_port.c
 * @brief   Ymodem 协议底层硬件接口实现
 * @note    提供串口收发 和 存储 (Flash) 操作的硬件抽象层。
 *          存储操作基于偏移量，协议层无需感知绝对地址。
 ******************************************************************************
 */

/* 包含头文件 ----------------------------------------------------------------*/
#include "ymodem_port.h"
#include "stm32f4xx.h"
#include "usart.h"
#include "flash.h"

/* ============================================================
 * 模块私有 (static) 变量 — 存储子系统
 * ============================================================ */

static uint32_t storage_base_addr;   /* 存储写入基准地址 */
static uint32_t storage_capacity;    /* 从基准地址到存储末尾的容量 (字节) */

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

/* ============================================================
 * 存储抽象层实现
 * ============================================================ */

#if FLASH_INTERNAL
/**
 * @brief  初始化存储子系统，设置写入基准地址
 * @param  base_address: 存储写入的起始地址
 * @note   计算可用容量 = Flash_TotalSize - (base_address - FLASH_BASE)
 *         内部 Flash 基地址 FLASH_BASE = 0x08000000
 */
void YmodemPort_StorageInit(uint32_t base_address)
{
    storage_base_addr = base_address;
    storage_capacity  = Get_Flash_Size() - (base_address - FLASH_BASE);
}

/**
 * @brief  获取从基准地址到存储末尾的可用容量 (字节)
 */
uint32_t YmodemPort_GetCapacity(void)
{
    return storage_capacity;
}

/**
 * @brief  擦除从基准地址开始的指定大小区域
 * @param  size: 要擦除的字节数 (从 storage_base_addr 开始)
 * @retval YMODEM_PORT_OK / YMODEM_PORT_ERR
 */
int32_t YmodemPort_StorageErase(uint32_t size)
{
    if (Erase_Area(storage_base_addr, size) == FLASH_COMPLETE)
    {
        return YMODEM_PORT_OK;
    }
    return YMODEM_PORT_ERR;
}

/**
 * @brief  从基准偏移处写入一块数据，并逐 Word 读回校验
 * @param  offset: 相对于 storage_base_addr 的字节偏移
 * @param  data:   数据源指针
 * @param  length: 要写入的字节数 (必须为 4 的倍数)
 * @retval YMODEM_PORT_OK / YMODEM_PORT_ERR
 *
 * @note   逐 Word (4 字节) 写入，每写入一个 Word 立即读回比对。
 *         若任一 Word 写入或校验失败，立即返回错误。
 */
int32_t YmodemPort_StorageWrite(uint32_t offset, const uint8_t *data, uint32_t length)
{
    uint32_t addr = storage_base_addr + offset;
    uint32_t i;

    for (i = 0; i < length; i += 4)
    {
        uint32_t word = *(uint32_t *)(data + i);

        /* 写入一个 Word */
        if (Program_Word(addr, word) != FLASH_COMPLETE)
        {
            return YMODEM_PORT_ERR;
        }

        /* 读回校验 */
        if (*(uint32_t *)addr != word)
        {
            return YMODEM_PORT_ERR;
        }

        addr += 4;
    }

    return YMODEM_PORT_OK;
}
#endif   /* FLASH_INTERNAL */


#if FLASH_EXTERNAL

#include "w25q64_handler.h"

/**
 * @brief  初始化存储子系统，设置写入基准偏移
 * @param  base_address: 外部 Flash 芯片内部偏移地址 (0 ~ 8MB-1)
 * @note   计算可用容量 = W25Q64_TotalSize - base_address
 */
void YmodemPort_StorageInit(uint32_t base_address)
{
    storage_base_addr = base_address;
    storage_capacity  = W25Q64_Handler_GetSize() - base_address;
}

/**
 * @brief  获取从基准偏移到芯片末尾的可用容量 (字节)
 */
uint32_t YmodemPort_GetCapacity(void)
{
    return storage_capacity;
}

/**
 * @brief  擦除从基准偏移开始的指定大小区域
 * @param  size: 要擦除的字节数 (从 storage_base_addr 开始)
 * @retval YMODEM_PORT_OK / YMODEM_PORT_ERR
 * @note   内部按 64KB Block 对齐擦除，由 W25Q64_Handler_Erase 处理
 */
int32_t YmodemPort_StorageErase(uint32_t size)
{
    if (W25Q64_Handler_Erase(storage_base_addr, size) == W25Q64_HANDLER_OK)
    {
        return YMODEM_PORT_OK;
    }
    return YMODEM_PORT_ERR;
}

/**
 * @brief  从基准偏移处写入一块数据，并读回校验
 * @param  offset: 相对于 storage_base_addr 的字节偏移
 * @param  data:   数据源指针
 * @param  length: 要写入的字节数
 * @retval YMODEM_PORT_OK / YMODEM_PORT_ERR
 *
 * @note   通过 SPI 写入外部 Flash (W25Q64_Handler_Write 内部自动处理页对齐)。
 *         写入完成后读取整个区域逐字节比对校验，确保数据完整性。
 *         校验使用 256 字节分块读取以平衡性能与栈空间占用。
 */
int32_t YmodemPort_StorageWrite(uint32_t offset, const uint8_t *data, uint32_t length)
{
    uint32_t addr = storage_base_addr + offset;
    uint8_t  verify_buf[256];
    uint32_t remaining, read_off;

    /* 写入外部 Flash (页编程 + 自动等待完成) */
    if (W25Q64_Handler_Write(addr, data, length) != W25Q64_HANDLER_OK)
    {
        return YMODEM_PORT_ERR;
    }

    /* 读回逐字节比对校验 */
    remaining = length;
    read_off  = 0;
    while (remaining > 0)
    {
        uint32_t chunk = (remaining > sizeof(verify_buf)) ? sizeof(verify_buf) : remaining;

        if (W25Q64_Handler_Read(addr + read_off, verify_buf, chunk) != W25Q64_HANDLER_OK)
        {
            return YMODEM_PORT_ERR;
        }

        for (uint32_t i = 0; i < chunk; i++)
        {
            if (verify_buf[i] != data[read_off + i])
            {
                return YMODEM_PORT_ERR;
            }
        }

        read_off  += chunk;
        remaining -= chunk;
    }

    return YMODEM_PORT_OK;
}

#endif   /* FLASH_EXTERNAL */





