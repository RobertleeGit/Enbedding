#include "usart.h"



static void Usart_GPIO_Init(void)
{
    // 1. GPIO 结构体初始化
    GPIO_InitTypeDef GPIO_InitStructure;
    // 打开两个 GPIO 时钟
    RCC_AHB1PeriphClockCmd(USART_TX_GPIO_CLK|USART_RX_GPIO_CLK, ENABLE);
 
	// 设置 GPIO 电阻为上拉
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    // 设置 GPIO 速度为 50MHz
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    // 设置输出模式为 推挽输出
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    // 设置 GPIO 模式为 复用模式
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; 
	
    // 选择 USART TX 的引脚 GPIO9
    GPIO_InitStructure.GPIO_Pin = USART_TX_PIN; 
	// 使用结构体初始化 USART TX 引脚
	GPIO_Init(USART_TX_GPIO_PORT, &GPIO_InitStructure);

    // 选择 USART RX 的引脚 GPIO10
    GPIO_InitStructure.GPIO_Pin = USART_RX_PIN; 
	// 使用结构体初始化 USART RX 引脚
	GPIO_Init(USART_RX_GPIO_PORT, &GPIO_InitStructure);


    // 2. 选择 GPIO 引脚的具体复用功能
    // 连接 PXx 到 USARTx_Tx
    GPIO_PinAFConfig(USART_TX_GPIO_PORT, USART_TX_SOURCE, USART_TX_AF);
    // 连接 PXx 到 USARTx_Rx
    GPIO_PinAFConfig(USART_RX_GPIO_PORT, USART_RX_SOURCE, USART_RX_AF);
}

static void Usart_Init(void)
{
    // 初始化 USART结构体
    USART_InitTypeDef USART_InitStructure;
    // 打开 USART1 的时钟
    RCC_APB2PeriphClockCmd(USART_CLK, ENABLE);
    // 设置 波特率 115200
    USART_InitStructure.USART_BaudRate = USART_BAUDRATE;
    // 设置数据位为 8 位
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    // 设置停止位
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    // 设置校验位（不用校验位）
    USART_InitStructure.USART_Parity = USART_Parity_No;
    // 不适用硬件流控制
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    // 设置 USART 模式，同时能发送和接收
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    // 使用结构体初始化 USART 
    USART_Init(USARTx, &USART_InitStructure);
    // 使能串口
    USART_Cmd(USARTx, ENABLE); 
}

void USART1_Configuration(void)
{
    Usart_GPIO_Init();
    Usart_Init();
}

void USART_SendChar(USART_TypeDef *Usartx, uint8_t data) {
    // 等待发送寄存器空
    while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET);
    // 发送一个字节
    USART_SendData(USARTx, (uint8_t)data);
}

uint16_t USART_ReceiveChar(USART_TypeDef *Usartx) {
    // 等待接收寄存器满
    while (USART_GetFlagStatus(USARTx, USART_FLAG_RXNE) == RESET);
    // 读取一个字节
    uint16_t data = USART_ReceiveData(USARTx);
    return data;
}



#ifdef __GNUC__
  #define PUTCHAR_PROTOTYPR int _io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPR int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

// 重定向 printf
PUTCHAR_PROTOTYPR {
    /* 等待发送寄存器空 */
    while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET);
    /* 发送一个字节数据到串口 */
    USART_SendData(USARTx, (uint8_t)ch);
    return ch;
}


