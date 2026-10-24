#ifndef LOADER_DIAG_H
#define LOADER_DIAG_H

#include "stm32h7xx_hal.h"
#include <stdint.h>

/*
 * Diagnostic area deliberately placed far above the loader image/buffer.
 *
 * Current CubeProgrammer log shows the loader around 0x24000004..0x240021xx,
 * while AXI SRAM extends to 0x24050000 in this loader layout.
 */
#define LOADER_DIAG_BASE   0x24040000UL
#define LOADER_DIAG_MAGIC  0x4C445236UL  /* "LDR6" marker */

#define LD_WORD(n) (*(volatile uint32_t *)(LOADER_DIAG_BASE + ((n) * 4U)))

/*
 * Word layout:
 *   [0] magic
 *   [1] top-level Init stage
 *   [2] detailed sub-stage
 *   [3] hospi2.State
 *   [4] hospi2.ErrorCode
 *   [5] JEDEC ID packed as 0x00MMTTCC
 *   [6] last Status Register-1 value
 *   [7] HAL tick snapshot
 *   [8] bit-bang JEDEC ID (0x00MMTTCC)
 *   [9] bit-bang MISO idle sample before command
 */
static inline void LoaderDiag_Reset(void)
{
    for (uint32_t i = 0; i < 24U; i++)
        LD_WORD(i) = 0U;

    LD_WORD(0) = LOADER_DIAG_MAGIC;
}

static inline void LoaderDiag_Top(uint32_t stage)
{
    LD_WORD(1) = stage;
    LD_WORD(7) = HAL_GetTick();
}

static inline void LoaderDiag_Sub(uint32_t stage)
{
    LD_WORD(2) = stage;
    LD_WORD(7) = HAL_GetTick();
}

static inline void LoaderDiag_OSPI(const OSPI_HandleTypeDef *h)
{
    if (h != NULL)
    {
        LD_WORD(3) = h->State;
        LD_WORD(4) = h->ErrorCode;
    }
}

static inline void LoaderDiag_Jedec(const uint8_t id[3])
{
    LD_WORD(5) = ((uint32_t)id[0] << 16)
               | ((uint32_t)id[1] << 8)
               | ((uint32_t)id[2]);
}

static inline void LoaderDiag_Status1(uint8_t sr)
{
    LD_WORD(6) = (uint32_t)sr;
}


static inline void LoaderDiag_BitbangJedec(const uint8_t id[3])
{
    LD_WORD(8) = ((uint32_t)id[0] << 16)
               | ((uint32_t)id[1] << 8)
               | ((uint32_t)id[2]);
}

static inline void LoaderDiag_BitbangIdle(uint32_t level)
{
    LD_WORD(9) = level;
}


static inline void LoaderDiag_OSPIRegs(void)
{
    /*
     * Extended v9 register snapshot.
     *
     * [10] OCTOSPIM->CR
     * [11] OCTOSPIM->PCR[0]
     * [12] OCTOSPIM->PCR[1]
     * [13] OCTOSPI2->CR
     * [14] OCTOSPI2->DCR1
     * [15] OCTOSPI2->DCR2
     * [16] OCTOSPI2->DCR3
     * [17] OCTOSPI2->DCR4
     * [18] OCTOSPI2->TCR
     * [19] OCTOSPI2->CCR
     * [20] OCTOSPI2->SR
     * [21] RCC->D1CCIPR
     * [22] RCC->AHB3ENR
     * [23] RCC->AHB3RSTR
     */
    LD_WORD(10) = OCTOSPIM->CR;
    LD_WORD(11) = OCTOSPIM->PCR[0];
    LD_WORD(12) = OCTOSPIM->PCR[1];

    LD_WORD(13) = OCTOSPI2->CR;
    LD_WORD(14) = OCTOSPI2->DCR1;
    LD_WORD(15) = OCTOSPI2->DCR2;
    LD_WORD(16) = OCTOSPI2->DCR3;
    LD_WORD(17) = OCTOSPI2->DCR4;
    LD_WORD(18) = OCTOSPI2->TCR;
    LD_WORD(19) = OCTOSPI2->CCR;
    LD_WORD(20) = OCTOSPI2->SR;

    LD_WORD(21) = RCC->D1CCIPR;
    LD_WORD(22) = RCC->AHB3ENR;
    LD_WORD(23) = RCC->AHB3RSTR;
}

#endif
