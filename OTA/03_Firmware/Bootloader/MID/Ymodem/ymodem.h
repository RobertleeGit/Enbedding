/**
 ******************************************************************************
 * @file    ymodem.h
 * @brief   Ymodem 协议头文件 — 仅接收 (Receiver)
 * @note    本文件仅包含 Ymodem 协议接收端所需的常量定义和函数声明。
 *          发送端 (Transmit) 相关功能已移除，以精简 Bootloader 代码。
 *
 *         Ymodem 协议握手流程 (接收端视角):
 *         1. 接收端发送 'C' (CRC16 请求) 等待发送端开始传输
 *         2. 发送端发送第 0 号包 (文件名 + 文件大小)
 *         3. 接收端回复 ACK + 'C'
 *         4. 发送端依次发送数据包 (序号从 1 开始)
 *         5. 接收端每收到一个正确包回复 ACK
 *         6. 发送端发送 EOT 表示传输结束
 *         7. 接收端回复 ACK
 *         8. 发送端发送空包 (全 0x00) 表示会话结束
 *         9. 接收端回复 ACK，传输完成
 ******************************************************************************
 */

#ifndef _YMODEM_H_
#define _YMODEM_H_

/* 包含头文件 ----------------------------------------------------------------*/
#include <stdint.h>

/* ============================================================
 * Ymodem 数据包结构定义
 *
 * 一个完整的 Ymodem 数据包结构：
 * +--------+--------+--------+-------------+-------+-------+
 * | SOH/STX| 序号   | 序号反码| 数据区      | CRC_H | CRC_L |
 * | 1 Byte | 1 Byte | 1 Byte | 128/1024 B | 1 Byte| 1 Byte|
 * +--------+--------+--------+-------------+-------+-------+
 * |<-- PACKET_HEADER -->|                            |<-TRAILER->|
 * |<--------------- PACKET_OVERHEAD --------------->|
 *
 * 数据包类型：
 * - SOH (0x01): 128 字节数据包
 * - STX (0x02): 1024 字节数据包 (Ymodem-1K 扩展)
 * - EOT (0x04): 传输结束 (无数据区)
 * ============================================================ */

/* --- 数据包字段偏移索引 --- */
#define PACKET_SEQNO_INDEX      (1)     /* 序号在包中的偏移 (从 0 开始计数) */
#define PACKET_SEQNO_COMP_INDEX (2)     /* 序号反码的偏移 (用于完整性校验) */

/* --- 数据包大小定义 --- */
#define PACKET_HEADER           (3)     /* 包头: SOH/STX + 序号 + 序号反码 = 3 字节 */
#define PACKET_TRAILER          (2)     /* 包尾: CRC16 高字节 + CRC16 低字节 = 2 字节 */
#define PACKET_OVERHEAD         (PACKET_HEADER + PACKET_TRAILER)  /* 包开销: 5 字节 */
#define PACKET_SIZE             (128)   /* 标准数据区大小 */
#define PACKET_1K_SIZE          (1024)  /* 1K 数据区大小 (Ymodem-1K) */

/* --- 文件名/大小字符串缓冲区 --- */
#define FILE_NAME_LENGTH        (256)   /* 文件名最大长度 */
#define FILE_SIZE_LENGTH        (16)    /* 文件大小字符串最大长度 (十进制) */

/* ============================================================
 * Ymodem 协议控制字符
 * ============================================================ */
#define SOH                     (0x01)  /* Start Of Header: 128 字节数据包起始标志 */
#define STX                     (0x02)  /* Start of Text:  1024 字节数据包起始标志 */
#define EOT                     (0x04)  /* End Of Transmission: 文件传输完成 */
#define ACK                     (0x06)  /* Acknowledge: 正确接收应答 */
#define NAK                     (0x15)  /* Negative Acknowledge: 接收错误/重传请求 */
#define CA                      (0x18)  /* Cancel: 连续两个 CA 表示取消传输 */
#define CRC16                   (0x43)  /* 'C' = 0x43: 请求发送端使用 CRC16 校验 */

/* --- 用户中止传输字符 --- */
#define ABORT1                  (0x41)  /* 'A': 用户中止 */
#define ABORT2                  (0x61)  /* 'a': 用户中止 */

/* ============================================================
 * 协议参数
 * ============================================================ */
#define NAK_TIMEOUT             (0x100000)  /* 接收超时阈值 (循环计数值，非精确毫秒) */
#define MAX_ERRORS              (10)         /* 最大连续错误次数，超过则中止传输 */

/* ============================================================
 * 状态枚举
 * ============================================================ */

/**
 * @brief Ymodem_Receive() 返回值枚举
 * @note  正值表示成功并携带文件大小 (字节数)，负值/零为错误码
 */
typedef enum {
    YMODEM_OK_CANCELLED     =  0,   /* 传输被取消或会话正常结束 */
    YMODEM_ERR_FILE_TOO_BIG = -1,   /* 文件大小超过存储可用空间 */
    YMODEM_ERR_STORAGE_FAIL = -2,   /* 存储擦除 / 编程 / 校验失败 */
    YMODEM_ERR_USER_ABORT   = -3    /* 用户主动中止传输 */
} Ymodem_Status;

/**
 * @brief Receive_Packet() 返回值枚举
 */
typedef enum {
    RECV_PACKET_OK      =  0,       /* 正常接收 (结合 packet_length 判断具体含义) */
    RECV_PACKET_ERR     = -1,       /* 超时或数据包格式错误 */
    RECV_PACKET_ABORT   =  1        /* 收到用户中止字符 (ABORT1/ABORT2) */
} RecvPacket_Status;

/**
 * @brief Receive_Packet() 中 packet_length 的特殊语义值
 * @note  正值表示实际数据载荷长度 (128 或 1024 字节)
 */
typedef enum {
    PACKET_LEN_CANCEL   = -1,       /* 发送端连续发送两个 CA，要求中止 */
    PACKET_LEN_EOT      =  0        /* 收到 EOT，文件传输结束 */
} PacketLen_Special;

/* ============================================================
 * 函数声明 — Ymodem 接收端
 * ============================================================ */

/**
 * @brief  Ymodem 协议接收文件 (Bootloader 核心入口)
 * @param  buf: 接收缓冲区指针 (至少需要 PACKET_1K_SIZE + PACKET_OVERHEAD 字节)
 * @retval >0:                            成功，返回接收到的文件总大小 (字节数)
 * @retval YMODEM_OK_CANCELLED     ( 0):  传输被取消或会话结束
 * @retval YMODEM_ERR_FILE_TOO_BIG (-1):  文件大小超过存储可用空间
 * @retval YMODEM_ERR_STORAGE_FAIL (-2):  存储擦除 / 编程 / 校验失败
 * @retval YMODEM_ERR_USER_ABORT   (-3):  用户中止传输
 *
 * @note   调用前必须先执行 YmodemPort_StorageInit() 设置写入目标。
 *         此函数是阻塞式的，会持续接收直到传输完成或出错。
 *         函数内部会自动完成存储擦除、写入和校验。
 *         协议层仅使用字节偏移量，不感知存储介质绝对地址。
 */
int32_t Ymodem_Receive(uint8_t *buf);

/**
 * @brief  更新 CRC16 值 (单字节增量计算)
 * @param  crcIn: 当前 CRC16 值
 * @param  byte:  新输入的字节
 * @retval 更新后的 CRC16 值
 *
 * @note   使用 CRC-16/XMODEM 多项式: x^16 + x^12 + x^5 + 1 (0x1021)
 *         此函数用于逐字节计算 CRC，配合 Cal_CRC16 使用。
 */
uint16_t UpdateCRC16(uint16_t crcIn, uint8_t byte);

/**
 * @brief  计算一块数据的 CRC16 校验值
 * @param  data: 数据指针
 * @param  size: 数据长度 (字节)
 * @retval 16 位 CRC 校验值
 *
 * @note   Ymodem 协议要求对每个数据包的 128/1024 字节数据区计算 CRC16，
 *         发送端会将 CRC16 附加在包尾，接收端用于校验数据完整性。
 */
uint16_t Cal_CRC16(const uint8_t *data, uint32_t size);

#endif /* _YMODEM_H_ */
