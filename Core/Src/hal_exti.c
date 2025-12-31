#include "hal_exti.h"
#include "button.h"

void HalExti_Init(void)
{
    // nothing for now
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == GPIO_PIN_0)
	  {
	    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
	  }
    Button_OnExti(GPIO_Pin);
}
