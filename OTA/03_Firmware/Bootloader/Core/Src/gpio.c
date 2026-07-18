#include "gpio.h"
  
void GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	  /* Key on pin(PA0) ****************************************/ 
	/* Enable the GPIOA peripheral */ 
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = Key_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;  
	GPIO_Init(Key_GPIO_Port, &GPIO_InitStructure);

	/* LED on C13 pin(PC13) ***********************************/ 
	/* Enable the GPIOCperipheral */ 
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	/* Configure C13 pin(PC13) in output function */
	GPIO_InitStructure.GPIO_Pin = LED_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;  
	GPIO_Init(LED_GPIO_Port, &GPIO_InitStructure);
}

uint8_t Key_Scan(void)
{
    if (GPIO_ReadInputDataBit(Key_GPIO_Port, Key_Pin) == Bit_RESET)
    {
        Delay(50);
        if (GPIO_ReadInputDataBit(Key_GPIO_Port, Key_Pin) == Bit_RESET)
        {
            return 1;
        }
    }
    return 0;
}

