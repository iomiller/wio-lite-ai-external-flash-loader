#include "Dev_Inf.h"
#include "loader_config.h"

#if defined(__ICCARM__)
__root struct StorageInfo const StorageInfo =
#else
struct StorageInfo __attribute__((section(".Dev_info"))) StorageInfo =
#endif
{
    "W25Q128JVSIQ_STM32H725AE_OSPI2",
    SPI_FLASH,
    EXT_FLASH_BASE_ADDRESS,
    EXT_FLASH_SIZE_BYTES,
    EXT_FLASH_PAGE_SIZE,
    0xFF,
    {
        { EXT_FLASH_SECTOR_COUNT, EXT_FLASH_SECTOR_SIZE },
        { 0x00000000, 0x00000000 }
    }
};
