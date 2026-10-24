#ifndef OSPI2_BOARD_H
#define OSPI2_BOARD_H

#include "stm32h7xx_hal.h"

extern OSPI_HandleTypeDef hospi2;

int OSPI2_BoardInit(void);
void OSPI2_BoardDeInit(void);

#endif
