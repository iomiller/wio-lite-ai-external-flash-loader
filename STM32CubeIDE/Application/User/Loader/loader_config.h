#ifndef LOADER_CONFIG_H
#define LOADER_CONFIG_H

#include "stm32h7xx_hal.h"

/*
 * STM32H725AE + W25Q128JVSIQ
 *
 * Board wiring supplied by user:
 *   W25Q IO0 -> PG0   = OCTOSPIM_P2_IO4, AF9
 *   W25Q IO1 -> PG1   = OCTOSPIM_P2_IO5, AF9
 *   W25Q IO2 -> PG10  = OCTOSPIM_P2_IO6, AF3
 *   W25Q IO3 -> PG11  = OCTOSPIM_P2_IO7, AF9
 *   W25Q CLK -> PF4   = OCTOSPIM_P2_CLK, AF9
 *   W25Q /CS -> PG12  = OCTOSPIM_P2_NCS, AF3
 *
 * Important:
 * The flash is wired to the HIGH physical nibble of OCTOSPIM port 2.
 * We therefore route the logical low IO[3:0] group to PORT_2_HIGH:
 *
 *   IOLowPort = HAL_OSPIM_IOPORT_2_HIGH
 *   IOHighPort = HAL_OSPIM_IOPORT_NONE
 *
 * The loader intentionally uses only 1-1-1 SPI commands at first.
 */

#define EXT_FLASH_BASE_ADDRESS   0x70000000UL
#define EXT_FLASH_SIZE_BYTES     0x01000000UL   /* 16 MiB */
#define EXT_FLASH_PAGE_SIZE      0x00000100UL   /* 256 B */
#define EXT_FLASH_SECTOR_SIZE    0x00001000UL   /* 4 KiB */
#define EXT_FLASH_SECTOR_COUNT   4096UL

#define LOADER_RAM_BASE          0x24000004UL
#define LOADER_VECTOR_BASE       0x24000200UL

#define LOADER_OK                1
#define LOADER_FAIL              0

#endif
