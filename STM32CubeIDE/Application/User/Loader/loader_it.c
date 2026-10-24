#include "stm32h7xx_hal.h"

/*
 * STM32CubeProgrammer enters Init() directly; HAL_Init() configures the
 * default HAL time base using SysTick.
 *
 * The STM32 startup file declares SysTick_Handler as a weak alias to
 * Default_Handler.  Without a strong implementation, the first SysTick
 * interrupt traps the loader forever in Default_Handler.
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
}
