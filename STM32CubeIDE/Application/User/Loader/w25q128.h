#ifndef W25Q128_H
#define W25Q128_H

#include "stm32h7xx_hal.h"
#include <stdint.h>

#define W25Q128_OK     0
#define W25Q128_ERROR -1

int W25Q128_Init(void);
int W25Q128_ReadJedecId(uint8_t id[3]);
int W25Q128_Read(uint32_t address, uint8_t *data, uint32_t size);
int W25Q128_PageProgram(uint32_t address, const uint8_t *data, uint32_t size);
int W25Q128_SectorErase4K(uint32_t address);
int W25Q128_ChipErase(void);
int W25Q128_WaitReady(uint32_t timeout_ms);
int W25Q128_EnableMemoryMappedMode(void);
int W25Q128_DisableMemoryMappedMode(void);

#endif
