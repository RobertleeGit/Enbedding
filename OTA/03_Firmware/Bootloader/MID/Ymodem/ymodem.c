/**
 ******************************************************************************
 * @file    ymodem.c
 * @brief   Ymodem 协议接收端 (Receiver) 完整实现
 * @note    本文件是 Bootloader OTA 升级的核心，通过串口 + Ymodem 协议
 *          接收上位机发送的固件镜像，并烧录到 STM32F411CE 内部 Flash。
 *
 *         【重要】本文件不含发送功能 (Transmit)，仅做接收。
 *
 * 依赖关系：
 *   ymodem.c
 *     |-- ymodem.h
 *     |-- ymodem_port.h 
 ******************************************************************************
 */

/* 包含头文件 ----------------------------------------------------------------*/
#include "ymodem_port.h" /* SerialKeyPressed, SerialPutChar, YmodemPort_FlashXxx */
#include "ymodem.h"     /* 协议常量: SOH, STX, EOT, ACK, NAK 等 */
#include <string.h>     /* memcpy */

/* ============================================================
 * 模块私有 (static) 函数声明
 * ============================================================ */

/* 带超时的单字节接收 */
static int32_t Receive_Byte(uint8_t *c, uint32_t timeout);
/* 接收一个完整的 Ymodem 数据包 */
static RecvPacket_Status Receive_Packet(uint8_t *data, int32_t *length, uint32_t timeout);
/* 发送一个字节 (封装，内部调用 SerialPutChar) */
static uint32_t Send_Byte(uint8_t c);
/* 处理第 0 号包：解析文件名 & 大小 → 校验空间 → 擦除 Flash → 应答 */
static int32_t Ymodem_ProcessPacket0(const uint8_t *packet_data, uint32_t starting_address);
/* 将一包数据写入 Flash 并逐 Word 校验 */
static Ymodem_Status Ymodem_FlashWritePacket(const uint8_t *src, 
                                             int32_t length,
                                             uint32_t starting_address, 
                                             int32_t total_size);

/* ============================================================
 * 模块私有 (static) 变量
 * ============================================================ */

/* Flash 当前写入地址 (在 Ymodem_Receive 中被修改) */
static uint32_t FlashDestination;

/**
 * @brief  带超时的单字节接收
 * @param  c:       输出参数，接收到的字节
 * @param  timeout: 超时计数 (轮询循环次数，非精确毫秒)
 * @retval  0: 成功接收到一个字节
 * @retval -1: 超时 (未收到数据)
 *
 * @note   通过不断轮询 SerialKeyPressed 实现非阻塞超时接收。
 *         每次循环 timeout 减 1，减到 0 则超时返回 -1。
 *         Ymodem 握手和包接收均依赖此超时机制。
 */
static int32_t Receive_Byte(uint8_t *c, uint32_t timeout)
{
    while (timeout-- > 0)
    {
        /* 非阻塞检查：有数据到达吗？ */
        if (SerialKeyPressed(c) == 1)
        {
            return 0;   /* 收到数据 */
        }
    }
    return -1;           /* 超时 */
}

/**
 * @brief  通过串口发送一个字节
 * @param  c: 要发送的字节
 * @retval 0: 发送完成
 *
 * @note   内部调用 common.c 中的 SerialPutChar。
 *         Ymodem 接收端主要发送: ACK(0x06), NAK(0x15), CA(0x18), CRC16(0x43)
 */
static uint32_t Send_Byte(uint8_t c)
{
    SerialPutChar(c);
    return 0;
}




/* ============================================================
 * Ymodem 数据包接收
 * ============================================================ */

/**
 * @brief  接收一个完整的 Ymodem 数据包
 * @param  data:    输出缓冲区，存放接收到的完整数据包
 * @param  length:  输出参数，数据包有效载荷长度
 *                  PACKET_LEN_CANCEL (-1): 发送端中止传输 (收到连续两个 CA)
 *                  PACKET_LEN_EOT    ( 0): 传输结束 (收到 EOT)
 *                  >0:                    数据包有效载荷字节数 (128 或 1024)
 * @param  timeout: 接收超时计数
 * @retval RECV_PACKET_OK    ( 0): 正常返回 (检查 length 值获取具体含义)
 * @retval RECV_PACKET_ERR   (-1): 超时或数据包格式错误
 * @retval RECV_PACKET_ABORT ( 1): 用户中止传输 (收到 ABORT1 或 ABORT2)
 *
 * @note   此函数解析 Ymodem 数据包的第一个字节来判断包类型：
 *         SOH -> 128 字节数据包
 *         STX -> 1024 字节数据包
 *         EOT -> 文件传输结束
 *         CA+CA -> 取消传输
 *         ABORT1/ABORT2 -> 用户中止
 *
 *         接收完成后会校验序号完整性。
 */
static RecvPacket_Status Receive_Packet(uint8_t *data, int32_t *length, uint32_t timeout)
{
    uint16_t i, packet_size;
    uint8_t c;

    *length = 0;

    /* 1. 接收第一个字节 — 判断包类型 */
    if (Receive_Byte(&c, timeout) != 0)
    {
        return RECV_PACKET_ERR;  /* 超时，未收到任何数据 */
    }

    switch (c)
    {
        case SOH:
            /* 128 字节标准数据包 */
            packet_size = PACKET_SIZE;
            break;

        case STX:
            /* 1024 字节数据包 (Ymodem-1K 扩展) */
            packet_size = PACKET_1K_SIZE;
            break;

        case EOT:
            /* 传输结束信号 (发送端已发送完所有数据) */
            *length = PACKET_LEN_EOT;
            return RECV_PACKET_OK;

        case CA:
            /* 可能取消传输：需要连续两个 CA */
            if ((Receive_Byte(&c, timeout) == 0) && (c == CA))
            {
                *length = PACKET_LEN_CANCEL;   /* 确认取消 */
                return RECV_PACKET_OK;
            }
            else
            {
                return RECV_PACKET_ERR;        /* 单个 CA 无效 */
            }

        case ABORT1:
        case ABORT2:
            /* 用户主动中止传输 (终端按下 'A' 或 'a') */
            return RECV_PACKET_ABORT;

        default:
            /* 未识别的包类型 */
            return RECV_PACKET_ERR;
    }

    /* 2. 存储第一个字节 (SOH 或 STX) */
    *data = c;

    /* 3. 接收剩余数据：序号(1B) + 序号反码(1B) + 数据区 + CRC16(2B) */
    for (i = 1; i < (packet_size + PACKET_OVERHEAD); i++)
    {
        if (Receive_Byte(data + i, timeout) != 0)
        {
            return RECV_PACKET_ERR;  /* 接收超时 */
        }
    }

    /* 4. 校验序号完整性: 序号 和 序号反码 应该互为按位取反 */
    if (data[PACKET_SEQNO_INDEX] != ((data[PACKET_SEQNO_COMP_INDEX] ^ 0xFF) & 0xFF))
    {
        return RECV_PACKET_ERR;  /* 序号校验失败 — 数据包损坏 */
    }

    /* 5. 正常返回，传出有效载荷长度 */
    *length = (int32_t)packet_size;
    return RECV_PACKET_OK;
}

/* ============================================================
 * Ymodem 接收辅助函数
 * ============================================================ */

/**
 * @brief  处理第 0 号包 (文件名 & 文件大小)
 * @note   负责：解析文件名/大小 → 校验 Flash 空间 → 擦除 Flash → 发送应答
 * @param  packet_data:     原始协议包数据
 * @param  starting_address: Flash 写入起始地址
 * @retval >0:                         成功，返回文件总大小 (字节)
 * @retval YMODEM_ERR_FILE_TOO_BIG (-1): 文件超出 Flash 容量
 * @retval YMODEM_ERR_FLASH_FAIL  (-2):  Flash 擦除失败
 */
static int32_t Ymodem_ProcessPacket0(const uint8_t *packet_data, uint32_t starting_address)
{
    uint8_t  file_size_str[FILE_SIZE_LENGTH];
    uint8_t *file_ptr;
    int32_t  i, size = 0;

    /* (a) 跳过文件名 (Bootloader 不需要保存文件名) */
    for (i = 0, file_ptr = (uint8_t *)packet_data + PACKET_HEADER;
         (*file_ptr != 0) && (i < FILE_NAME_LENGTH);)
    {
        i++;
        file_ptr++;
    }

    /* (b) 解析文件大小字符串 (跳过文件名末尾的 '\0') */
    file_ptr++;  /* 跳过 '\0' */
    for (i = 0; (*file_ptr != ' ') && (*file_ptr != '\0') && (i < FILE_SIZE_LENGTH - 1);)
    {
        file_size_str[i++] = *file_ptr++;
    }
    file_size_str[i] = '\0';

    /* 手动解析十进制 → 整数 (避免引入 stdlib) */
    for (i = 0; file_size_str[i] >= '0' && file_size_str[i] <= '9'; i++)
    {
        size = size * 10 + (file_size_str[i] - '0');
    }

    uint32_t flash_size = YmodemPort_GetFlashSize();
    /* (c) 检查文件大小是否超出可用 Flash */
    if (size > (flash_size - (starting_address - 0x08000000)))
    {
        Send_Byte(CA);
        Send_Byte(CA);
        return YMODEM_ERR_FILE_TOO_BIG;
    }

    /* (d) 擦除 Flash 目标区域 (STM32F4 要求写入前必须先擦除) */
    if (YmodemPort_FlashErase(starting_address, (uint32_t)size) != YMODEM_PORT_OK)
    {
        Send_Byte(CA);
        Send_Byte(CA);
        return YMODEM_ERR_FLASH_FAIL;
    }

    /* (e) 回复 ACK + CRC16 请求，通知发送端开始传输数据 */
    Send_Byte(ACK);
    Send_Byte(CRC16);

    return size;  /* 返回文件总大小 */
}

/**
 * @brief  将一包数据写入 Flash 并逐 Word 校验
 * @note   每写入一个 Word (4 字节) 后立即读回比对，确保写入正确。
 *         同时更新模块级静态变量 FlashDestination。
 * @param  src:              RAM 中的数据源指针
 * @param  length:           本包有效载荷长度 (128 或 1024 字节)
 * @param  starting_address: Flash 基地址 (用于越界检查)
 * @param  total_size:       文件总大小 (用于越界检查)
 * @retval YMODEM_OK_CANCELLED  ( 0): 写入完成
 * @retval YMODEM_ERR_FLASH_FAIL (-2): 编程或校验失败
 */
static Ymodem_Status Ymodem_FlashWritePacket(const uint8_t *src, 
                                             int32_t length,
                                             uint32_t starting_address,
                                             int32_t total_size)
{
    uint32_t ram_addr = (uint32_t)src;
    int32_t  i;

    for (i = 0; (i < length) && (FlashDestination < starting_address + total_size); i += 4)
    {
        /* 写入一个 Word */
        if (YmodemPort_FlashWriteWord(FlashDestination, *(uint32_t *)ram_addr) != YMODEM_PORT_OK)
        {
            Send_Byte(CA);
            Send_Byte(CA);
            return YMODEM_ERR_FLASH_FAIL;
        }

        /* 读回校验 */
        if (YmodemPort_FlashVerify(FlashDestination, *(uint32_t *)ram_addr) != YMODEM_PORT_OK)
        {
            Send_Byte(CA);
            Send_Byte(CA);
            return YMODEM_ERR_FLASH_FAIL;
        }

        FlashDestination += 4;
        ram_addr         += 4;
    }

    Send_Byte(ACK);  /* 本包写入完成，应答发送端 */
    return YMODEM_OK_CANCELLED;  /* 0 — 正常 */
}

/* ============================================================
 * Ymodem 接收主函数
 * ============================================================ */

/**
 * @brief  Ymodem 协议接收文件
 * @param  buf[in]: 接收缓冲区指针 (至少 PACKET_1K_SIZE + PACKET_OVERHEAD 字节)
 * @param  starting_address[in]: Flash 写入起始地址 (必须是有效的 Flash 区域)
 * @param  size[out]: 输出参数，接收到的文件总大小 (字节)
 * @retval >0:                           成功，返回接收到的文件总大小 (字节数)
 * @retval YMODEM_OK_CANCELLED   ( 0):   传输被取消或无文件传输
 * @retval YMODEM_ERR_FILE_TOO_BIG (-1): 文件大小超过 Flash 可用空间
 * @retval YMODEM_ERR_FLASH_FAIL  (-2):  Flash 写入校验失败 (写入后读回不一致)
 * @retval YMODEM_ERR_USER_ABORT  (-3):  用户中止传输
 *
 * @note   完整的 Ymodem 接收流程：
 *
 *         【阶段 1 — 握手等待】
 *         循环发送 'C' (CRC16请求)，直到发送端开始发送第 0 号包。
 *
 *         【阶段 2 — 接收文件名包 (第 0 号包)】
 *         调用 Ymodem_ProcessPacket0() 解析并擦除 Flash。
 *
 *         【阶段 3 — 接收数据包 (第 1 ~ N 号包)】
 *         逐包接收 128/1024 字节数据，调用 Ymodem_FlashWritePacket() 写入 Flash。
 *         每包校验序号和 CRC16，正确则回复 ACK，错误则回复 NAK。
 *
 *         【阶段 4 — 传输结束】
 *         收到 EOT 回复 ACK，收到空文件名包确认会话结束。
 */
int32_t Ymodem_Receive(uint8_t *buf, uint32_t starting_address)
{
    uint8_t  packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];
    uint8_t *buf_ptr;
    int32_t  packet_length;
    int32_t  file_done, session_done;
    int32_t  packets_received, errors, session_begin;
    int32_t  size = 0;

    FlashDestination = starting_address;

    /* ============================================================
     * 外层循环：Ymodem 会话 (本实现仅处理单文件)
     * ============================================================ */
    for (session_done = 0, errors = 0, session_begin = 0; ;)
    {
        /* ============================================================
         * 内层循环：接收单个文件的所有数据包
         * ============================================================ */
        for (packets_received = 0, file_done = 0, buf_ptr = buf; ;)
        {
            /* --- 等待并接收一个 Ymodem 数据包 --- */
            switch (Receive_Packet(packet_data, &packet_length, NAK_TIMEOUT))
            {
                /* === 成功接收到包 === */
                case RECV_PACKET_OK:
                    errors = 0;

                    switch (packet_length)
                    {
                        case PACKET_LEN_CANCEL:
                            /* 发送端要求中止 */
                            Send_Byte(ACK);
                            return YMODEM_OK_CANCELLED;

                        case PACKET_LEN_EOT:
                            /* 传输结束 (收到 EOT)
                             * 发送 ACK + CRC16('C') 通知发送端准备接收空文件名包，*/
                            Send_Byte(ACK);
                            Send_Byte(CRC16);
                            file_done = 1;
                            break;

                        default:
                            /* 正常数据包：校验序号 */
                            if ((packet_data[PACKET_SEQNO_INDEX] & 0xFF)
                                    != (packets_received & 0xFF))
                            {
                                Send_Byte(NAK);  /* 序号不匹配 → 重传 */
                            }
                            else
                            {
                                if (packets_received == 0)
                                {
                                    /* --- 第 0 号包：文件名 + 大小 --- */
                                    if (packet_data[PACKET_HEADER] != 0)
                                    {
                                        int32_t result;
                                        result = Ymodem_ProcessPacket0(packet_data, starting_address);
                                        if (result < 0)
                                        {
                                            return result;  /* 错误码直接返回 */
                                        }
                                        size = result;
                                    }
                                    else
                                    {
                                        /* 空包：无文件，会话结束 */
                                        Send_Byte(ACK);
                                        file_done    = 1;
                                        session_done = 1;
                                        break;
                                    }
                                }
                                else
                                {
                                    /* --- 第 1 ~ N 号包：数据写入 Flash --- */
                                    Ymodem_Status status;

                                    /* 将数据从协议包拷贝到用户缓冲区 */
                                    memcpy(buf_ptr, packet_data + PACKET_HEADER, packet_length);

                                    /* 写入 Flash + 校验 */
                                    status = Ymodem_FlashWritePacket(buf, packet_length, starting_address, size);
                                    if (status != YMODEM_OK_CANCELLED)
                                    {
                                        return status;
                                    }
                                }

                                packets_received++;
                                session_begin = 1;
                            }
                            break;
                    }
                    break;

                /* === 用户主动中止 === */
                case RECV_PACKET_ABORT:
                    Send_Byte(CA);
                    Send_Byte(CA);
                    return YMODEM_ERR_USER_ABORT;

                /* === 超时 / 数据包损坏 === */
                default:
                    if (session_begin > 0)
                    {
                        errors++;
                    }
                    if (errors > MAX_ERRORS)
                    {
                        Send_Byte(CA);
                        Send_Byte(CA);
                        return YMODEM_OK_CANCELLED;
                    }
                    /* 请求发送端重试 */
                    Send_Byte(CRC16);
                    break;
            }

            /* 当前文件传输完成，跳出内层循环 */
            if (file_done != 0)
            {
                break;
            }
        }

        if (session_done != 0)
        {
            break;
        }
    }
    return (int32_t) size;
}

/* ============================================================
 * CRC16 校验函数
 * ============================================================ */

/**
 * @brief  更新 CRC16 值 — 单字节增量计算
 * @param  crcIn: 当前 CRC16 值 (初始值通常为 0)
 * @param  byte:  新输入的字节
 * @retval 更新后的 16 位 CRC 值
 *
 * @note   使用 CRC-16/XMODEM 标准多项式: x^16 + x^12 + x^5 + 1
 *         即多项式值 = 0x1021
 *
 *         此函数通过位操作实现 CRC 计算，无需查表。
 *         常用于逐字节流式计算 CRC。
 */
uint16_t UpdateCRC16(uint16_t crcIn, uint8_t byte)
{
    uint32_t crc = crcIn;
    uint32_t in = byte | 0x100;     /* 在数据前加一个 ''1'' 作为终止判断位 */

    do
    {
        crc <<= 1;                  /* CRC 左移 1 位 */
        in <<= 1;                   /* 数据左移 1 位 */
        if (in & 0x100)             /* 如果移出的 bit 是 1 */
            ++crc;                  /* CRC 最低位 +1 */
        if (crc & 0x10000)          /* 如果 CRC 第 17 位是 1 */
            crc ^= 0x1021;          /* 异或多項式 */
    }
    while (!(in & 0x10000));        /* 直到数据全部处理完 (''1'' 移到第 17 位) */

    return crc & 0xFFFFu;           /* 返回低 16 位 */
}

/**
 * @brief  计算一块数据的 CRC16 校验值
 * @param  data: 数据指针
 * @param  size: 数据长度 (字节)
 * @retval 16 位 CRC 校验值
 *
 * @note   对数据区逐字节计算 CRC16，最后追加两个空字节 (0x00) 作为结束。
 *         这是 Ymodem 协议规定的 CRC16 计算方法。
 *
 *         典型用法: 在接收到数据包后，对数据区计算 CRC16，
 *         与包尾的 CRC16 比较以验证数据完整性。
 */
uint16_t Cal_CRC16(const uint8_t *data, uint32_t size)
{
    uint32_t crc = 0;
    const uint8_t *dataEnd = data + size;

    /* 逐字节更新 CRC */
    while (data < dataEnd)
        crc = UpdateCRC16(crc, *data++);

    /* Ymodem 协议要求 CRC 计算末尾追加两个 0x00 字节 */
    crc = UpdateCRC16(crc, 0);
    crc = UpdateCRC16(crc, 0);

    return crc & 0xFFFFu;
}
