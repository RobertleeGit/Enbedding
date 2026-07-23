/* Includes ------------------------------------------------------------------*/
#include <stdio.h>

#include "main.h"
#include "tim.h"
#include "gpio.h"
#include "usart.h"
#include "flash.h"
#include "ymodem.h"
#include "ymodem_port.h"
#include "spi.h"
#include "w25q64_handler.h"

#include "Debug.h"
#include "boot_manage.h"

// 当前定义 STM32F411xE

// STM32F411 外部晶振25Mhz，考虑到USB使用，内部频率设置为96Mhz
// 需要100mhz,自行修改system_stm32f4xx.c

/** @addtogroup Template_Project
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static __IO uint32_t uwTimingDelay;
__IO uint32_t uwTick = 0;
RCC_ClocksTypeDef RCC_Clocks;

uint8_t ymodem_buf[1029];
int size = 11;
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

 /*
  *power by WeAct Studio
  *The board with `WeAct` Logo && `version number` is our board, quality guarantee. 
  *For more information please visit: https://github.com/WeActTC/MiniF4-STM32F4x1
  *更多信息请访问：https://gitee.com/WeActTC/MiniF4-STM32F4x1
  */
/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
	/* Enable Clock Security System(CSS): this will generate an NMI exception
	 when HSE clock fails *****************************************************/
	RCC_ClockSecuritySystemCmd(ENABLE);

	/*!< At this stage the microcontroller clock setting is already configured, 
	   this is done through SystemInit() function which is called from startup
	   files before to branch to application main.
	   To reconfigure the default setting of SystemInit() function, 
	   refer to system_stm32f4xx.c file */

	/* SysTick end of count event each 1ms */
	SystemCoreClockUpdate();
	RCC_GetClocksFreq(&RCC_Clocks);
	SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);

	/************************** Add your application code here **************************/	
  // TIM_Config();
  /* 初始化 Key 和 LED */
	GPIO_Config();
  /* 初始化 USART1 */
  USART1_Configuration();
  /* 初始化 EasyLogger */
  EasyLogger_Init();
  /* 初始化 SPI */
  SPI_Flash_Init();
  /* 初始化 W25Q64 */
  W25Q64_Handler_Init();

	// test
	LED_OFF;
	/* Infinite loop */
	while (1)
	{
    // 按键松开直接跳转到 APP
    if (0 == Key_Scan())
    {
      // jump_to_app(APP_START_ADDRESS);
    }
    else  // 按键按下
    {
      // 初始化 Ymodem 存储目标 (内部 Flash 备份区)
      YmodemPort_StorageInit(APP_BACK_ADDRESS);
      // 阻塞接收 Ymodem 升级文件到备份区
      size = Ymodem_Receive(ymodem_buf);
      // log_d("app size = %d", size);
      // 将备份区 Flash 的 APP 镜像拷贝到 APP 区
      copy_status_t copy_status = decodeCopy_back_to_app(APP_BACK_ADDRESS, APP_START_ADDRESS, size);
      if (copy_status == COPY_SUCCESS)
      {
        log_i("copy back to app success!");
        jump_to_app(APP_START_ADDRESS);
      }
      else
      {
        log_e("copy back to app failed!");
      }
    }
    // log_i("no valid app, please press key to upgrade!");
    Delay(50);
	}
}

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in milliseconds.
  * @retval None
  */
void Delay(__IO uint32_t nTime)
{ 
	uwTimingDelay = nTime;

	while(uwTimingDelay != 0);
}

/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
	if (uwTimingDelay != 0x00)
	{ 
		uwTimingDelay--;
	}
}

/**
  * @brief  Get current tick value in milliseconds.
  * @retval Current tick
  */
uint32_t GetTick(void)
{
    return uwTick;
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
