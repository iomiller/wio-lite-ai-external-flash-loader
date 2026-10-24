#include "ospi2_board.h"
#include "loader_config.h"
#include "loader_diag.h"
#include <string.h>

OSPI_HandleTypeDef hospi2;

static void OSPI2_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    /*
     * PF4 = OCTOSPIM_P2_CLK
     * STM32H725 alternate function: AF9
     */
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_OCTOSPIM_P2;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /*
     * AF9 group:
     *
     * PG0  = OCTOSPIM_P2_IO4
     * PG1  = OCTOSPIM_P2_IO5
     * PG11 = OCTOSPIM_P2_IO7
     */
    GPIO_InitStruct.Pin =
        GPIO_PIN_0  |
        GPIO_PIN_1  |
        GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_OCTOSPIM_P2;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    /*
     * AF3 group:
     *
     * PG10 = OCTOSPIM_P2_IO6
     * PG12 = OCTOSPIM_P2_NCS
     *
     * IMPORTANT:
     * These two STM32H725 pins are AF3, NOT AF9.
     */
    GPIO_InitStruct.Pin =
        GPIO_PIN_10 |
        GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF3_OCTOSPIM_P2;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
}

int OSPI2_BoardInit(void)
{
    OSPIM_CfgTypeDef ospim = {0};

    memset(&hospi2, 0, sizeof(hospi2));

    /*
     * Explicitly select the OCTOSPI kernel clock when this CubeH7 HAL exposes
     * the selector symbols.  The #if keeps the source compatible with HAL
     * packages that use a different RCC naming variant.
     */
#if defined(RCC_PERIPHCLK_OSPI) && defined(RCC_OSPICLKSOURCE_D1HCLK)
    {
        RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_OSPI;
        PeriphClkInitStruct.OspiClockSelection = RCC_OSPICLKSOURCE_D1HCLK;

        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
        {
            LoaderDiag_Sub(0x0000E101U);
            return LOADER_FAIL;
        }
    }
#endif

    __HAL_RCC_OSPI2_CLK_ENABLE();

    /*
     * CRITICAL: OCTOSPIM/I/O Manager has its own AHB3 register-interface
     * clock on STM32H725.
     *
     * __HAL_RCC_OSPI2_CLK_ENABLE() enables OCTOSPI2 only.
     * HAL_OSPIM_Config() accesses OCTOSPIM registers and therefore requires
     * the separate OCTOSPIM clock.
     */
#if defined(OCTOSPIM)
    __HAL_RCC_OCTOSPIM_CLK_ENABLE();
#endif

    /*
     * Put OCTOSPI2 into a known state.  A CubeProgrammer external loader can
     * be entered after arbitrary previous application firmware, so do not rely
     * on peripheral reset state.
     */
#if defined(__HAL_RCC_OSPI2_FORCE_RESET) && defined(__HAL_RCC_OSPI2_RELEASE_RESET)
    __HAL_RCC_OSPI2_FORCE_RESET();
    __DSB();
    __HAL_RCC_OSPI2_RELEASE_RESET();
    __DSB();
#endif

    OSPI2_GPIO_Init();

    /*
     * HSI after reset is sufficient for a loader.
     * Keep OSPI intentionally slow.
     *
     * DeviceSize = 23 means 24 address bits (16 MiB).
     */
    hospi2.Instance = OCTOSPI2;
    hospi2.Init.FifoThreshold = 1;
    hospi2.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
    hospi2.Init.MemoryType = HAL_OSPI_MEMTYPE_MICRON;
    hospi2.Init.DeviceSize = 24;
    hospi2.Init.ChipSelectHighTime = 2;
    hospi2.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
    hospi2.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
    hospi2.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
    hospi2.Init.ClockPrescaler = 7; /* deliberately conservative */
    hospi2.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;
    hospi2.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_DISABLE;
    hospi2.Init.ChipSelectBoundary = 0;
    hospi2.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_BYPASSED;
    hospi2.Init.MaxTran = 0;
    hospi2.Init.Refresh = 0;

    LoaderDiag_Sub(0x00000110U); /* before HAL_OSPI_Init */
    if (HAL_OSPI_Init(&hospi2) != HAL_OK)
    {
        LoaderDiag_Sub(0x0000E110U);
        LoaderDiag_OSPI(&hospi2);
        return LOADER_FAIL;
    }
    LoaderDiag_Sub(0x00000120U); /* HAL_OSPI_Init OK */
    LoaderDiag_OSPI(&hospi2);
    LoaderDiag_OSPIRegs();

    /*
     * Critical board-specific mapping:
     * logical OSPI IO[3:0] -> physical OCTOSPIM port-2 HIGH group IO[7:4].
     *
     * This makes:
     * logical IO0 -> PG0  / P2_IO4
     * logical IO1 -> PG1  / P2_IO5
     * logical IO2 -> PG10 / P2_IO6
     * logical IO3 -> PG11 / P2_IO7
     */
    ospim.ClkPort = 2;
    ospim.DQSPort = 0;
    ospim.NCSPort = 2;
    ospim.IOLowPort = HAL_OSPIM_IOPORT_2_HIGH;
    ospim.IOHighPort = HAL_OSPIM_IOPORT_NONE;
#if defined(OCTOSPIM_CR_MUXEN)
    ospim.Req2AckTime = 1;
#endif

    LoaderDiag_Sub(0x00000130U); /* before HAL_OSPIM_Config */
    if (HAL_OSPIM_Config(&hospi2, &ospim, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        LoaderDiag_Sub(0x0000E130U);
        LoaderDiag_OSPI(&hospi2);
        return LOADER_FAIL;
    }

    LoaderDiag_Sub(0x00000140U); /* board OSPI config complete */
    LoaderDiag_OSPI(&hospi2);
    LoaderDiag_OSPIRegs();
    return LOADER_OK;
}

void OSPI2_BoardDeInit(void)
{
    HAL_OSPI_DeInit(&hospi2);

    HAL_GPIO_DeInit(GPIOF, GPIO_PIN_4);
    HAL_GPIO_DeInit(GPIOG,
                    GPIO_PIN_0 |
                    GPIO_PIN_1 |
                    GPIO_PIN_10 |
                    GPIO_PIN_11 |
                    GPIO_PIN_12);

    __HAL_RCC_OSPI2_CLK_DISABLE();

#if defined(OCTOSPIM)
    __HAL_RCC_OCTOSPIM_CLK_DISABLE();
#endif
}
