#include "stm32h7xx_hal.h"
#include "w25q128_bitbang.h"
#include "loader_diag.h"

/*
 * Physical board wiring:
 *   PG12 = CS#
 *   PF4  = CLK
 *   PG0  = IO0 / MOSI
 *   PG1  = IO1 / MISO
 *   PG10 = IO2 / WP#
 *   PG11 = IO3 / HOLD#
 *
 * This test intentionally does NOT use OCTOSPI/OCTOSPIM.
 */

static inline void bb_delay(void)
{
    /* Deliberately slow; exact timing is not important for JEDEC-ID read. */
    for (volatile uint32_t i = 0; i < 100U; ++i)
    {
        __NOP();
    }
}

static void bb_gpio_init(void)
{
    GPIO_InitTypeDef g = {0};

    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    /* CLK low before enabling output. */
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_4, GPIO_PIN_RESET);

    g.Pin = GPIO_PIN_4;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOF, &g);

    /* CS high, MOSI low, IO2/IO3 high. */
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_12, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10 | GPIO_PIN_11, GPIO_PIN_SET);

    g.Pin = GPIO_PIN_0 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOG, &g);

    /*
     * MISO input. Pull-up makes "flash not driving" distinguishable:
     * disconnected/high-Z tends to read FF rather than 00.
     */
    g.Pin = GPIO_PIN_1;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOG, &g);

    bb_delay();
}

static uint8_t bb_xfer(uint8_t out)
{
    uint8_t in = 0U;

    for (int bit = 7; bit >= 0; --bit)
    {
        HAL_GPIO_WritePin(GPIOG,
                         GPIO_PIN_0,
                         ((out >> bit) & 1U) ? GPIO_PIN_SET : GPIO_PIN_RESET);

        bb_delay();

        /* SPI mode 0: sample on rising edge. */
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_4, GPIO_PIN_SET);
        bb_delay();

        in <<= 1;
        if (HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_1) == GPIO_PIN_SET)
            in |= 1U;

        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_4, GPIO_PIN_RESET);
        bb_delay();
    }

    return in;
}

void W25Q128_BitBangReadJedec(uint8_t id[3])
{
    bb_gpio_init();

    LoaderDiag_BitbangIdle(
        HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_1) == GPIO_PIN_SET ? 1U : 0U);

    /* Clean CS framing. */
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_12, GPIO_PIN_SET);
    bb_delay();
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_12, GPIO_PIN_RESET);
    bb_delay();

    (void)bb_xfer(0x9FU);
    id[0] = bb_xfer(0x00U);
    id[1] = bb_xfer(0x00U);
    id[2] = bb_xfer(0x00U);

    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_12, GPIO_PIN_SET);
    bb_delay();

    LoaderDiag_BitbangJedec(id);
}
