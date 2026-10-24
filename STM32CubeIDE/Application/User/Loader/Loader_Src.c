#include "loader_config.h"
#include "ospi2_board.h"
#include "w25q128.h"
#include "loader_diag.h"
#include "w25q128_bitbang.h"

#include <stdint.h>
#include <string.h>

static uint32_t to_flash_offset(uint32_t address)
{
    return address - EXT_FLASH_BASE_ADDRESS;
}

static int address_range_valid(uint32_t address, uint32_t size)
{
    uint32_t offset;

    if (address < EXT_FLASH_BASE_ADDRESS)
        return 0;

    offset = address - EXT_FLASH_BASE_ADDRESS;

    if (offset >= EXT_FLASH_SIZE_BYTES)
        return 0;

    if (size > (EXT_FLASH_SIZE_BYTES - offset))
        return 0;

    return 1;
}

/*
 * STM32CubeProgrammer loader entry point.
 *
 * Important:
 * CubeProgrammer performs direct AP reads from the advertised external
 * memory range. Therefore OCTOSPI2 must be in memory-mapped mode before
 * Init() returns.
 */
__attribute__((used))
int Init(void)
{
    LoaderDiag_Reset();
    LoaderDiag_Top(0x00000010U); /* entered Init */

    SystemInit();

    SCB->VTOR = LOADER_VECTOR_BASE;
    __DSB();
    __ISB();

    __set_PRIMASK(0);

    HAL_Init();
    LoaderDiag_Top(0x00000020U); /* HAL initialized */

    /*
     * Physical-pin sanity test before OCTOSPI takes ownership of the GPIOs.
     * Result is stored at 0x24040020.
     */
    {
        uint8_t bb_id[3] = {0U, 0U, 0U};
        W25Q128_BitBangReadJedec(bb_id);
    }

    LoaderDiag_OSPI(&hospi2);

    LoaderDiag_Top(0x00000030U); /* entering board init */
    if (OSPI2_BoardInit() != LOADER_OK)
    {
        LoaderDiag_Top(0x0000E001U); /* board init failed */
        LoaderDiag_OSPI(&hospi2);
        return LOADER_FAIL;
    }

    LoaderDiag_Top(0x00000040U); /* board init OK */
    LoaderDiag_OSPI(&hospi2);

    LoaderDiag_Top(0x00000050U); /* entering W25 init */
    if (W25Q128_Init() != W25Q128_OK)
    {
        LoaderDiag_Top(0x0000E002U); /* W25 init failed */
        LoaderDiag_OSPI(&hospi2);
        return LOADER_FAIL;
    }

    LoaderDiag_Top(0x00000060U); /* W25 init OK */
    LoaderDiag_OSPI(&hospi2);

    LoaderDiag_Top(0x00000070U); /* entering memory-mapped setup */
    if (W25Q128_EnableMemoryMappedMode() != W25Q128_OK)
    {
        LoaderDiag_Top(0x0000E003U); /* memory-mapped setup failed */
        LoaderDiag_OSPI(&hospi2);
        return LOADER_FAIL;
    }

    LoaderDiag_Top(0x00000080U); /* complete */
    LoaderDiag_OSPI(&hospi2);

    return LOADER_OK;
}

/*
 * Read implementation for loader API.
 *
 * The flash is already memory-mapped at 0x70000000, so use direct memory
 * reads rather than issuing another indirect OSPI command.
 */
__attribute__((used))
int Read(uint32_t Address, uint32_t Size, uint8_t *Buffer)
{
    if (!address_range_valid(Address, Size))
        return LOADER_FAIL;

    memcpy(Buffer, (const void *)Address, Size);

    return LOADER_OK;
}

__attribute__((used))
int Write(uint32_t Address, uint32_t Size, uint8_t *buffer)
{
    uint32_t offset;
    uint32_t page_remaining;
    uint32_t chunk;

    if (!address_range_valid(Address, Size))
        return LOADER_FAIL;

    /*
     * Leave memory-mapped mode before issuing indirect program commands.
     */
    if (HAL_OSPI_Abort(&hospi2) != HAL_OK)
        return LOADER_FAIL;

    offset = to_flash_offset(Address);

    while (Size > 0U)
    {
        page_remaining =
            EXT_FLASH_PAGE_SIZE -
            (offset & (EXT_FLASH_PAGE_SIZE - 1U));

        chunk = (Size < page_remaining) ? Size : page_remaining;

        if (W25Q128_PageProgram(offset, buffer, chunk) != W25Q128_OK)
            return LOADER_FAIL;

        offset += chunk;
        buffer += chunk;
        Size -= chunk;
    }

    if (W25Q128_EnableMemoryMappedMode() != W25Q128_OK)
        return LOADER_FAIL;

    return LOADER_OK;
}

__attribute__((used))
int SectorErase(uint32_t EraseStartAddress, uint32_t EraseEndAddress)
{
    uint32_t address;

    if (EraseStartAddress > EraseEndAddress)
        return LOADER_FAIL;

    if (!address_range_valid(EraseStartAddress, 1U))
        return LOADER_FAIL;

    if (!address_range_valid(EraseEndAddress, 1U))
        return LOADER_FAIL;

    if (HAL_OSPI_Abort(&hospi2) != HAL_OK)
        return LOADER_FAIL;

    address = EraseStartAddress & ~(EXT_FLASH_SECTOR_SIZE - 1U);

    while (address <= EraseEndAddress)
    {
        if (W25Q128_SectorErase4K(to_flash_offset(address)) != W25Q128_OK)
            return LOADER_FAIL;

        if (address > (0xFFFFFFFFUL - EXT_FLASH_SECTOR_SIZE))
            break;

        address += EXT_FLASH_SECTOR_SIZE;
    }

    if (W25Q128_EnableMemoryMappedMode() != W25Q128_OK)
        return LOADER_FAIL;

    return LOADER_OK;
}

__attribute__((used))
int MassErase(void)
{
    if (W25Q128_DisableMemoryMappedMode() != W25Q128_OK)
        return LOADER_FAIL;

    if (W25Q128_ChipErase() != W25Q128_OK)
    {
        (void)W25Q128_EnableMemoryMappedMode();
        return LOADER_FAIL;
    }

    if (W25Q128_EnableMemoryMappedMode() != W25Q128_OK)
        return LOADER_FAIL;

    return LOADER_OK;
}

/*
 * CubeProgrammer checksum.
 * Since the flash is memory-mapped during normal loader idle/read state,
 * checksum can operate directly on the mapped bytes.
 */
__attribute__((used))
uint32_t CheckSum(uint32_t StartAddress, uint32_t Size, uint32_t InitVal)
{
    const volatile uint8_t *p;
    uint32_t sum = InitVal;

    if (!address_range_valid(StartAddress, Size))
        return 0U;

    p = (const volatile uint8_t *)StartAddress;

    while (Size--)
        sum += *p++;

    return sum;
}

__attribute__((used))
int Verify(uint32_t MemoryAddr, uint32_t RAMBufferAddr, uint32_t Size)
{
    const volatile uint8_t *flash;
    const uint8_t *ram;
    uint32_t i;

    if (!address_range_valid(MemoryAddr, Size))
        return LOADER_FAIL;

    flash = (const volatile uint8_t *)MemoryAddr;
    ram = (const uint8_t *)RAMBufferAddr;

    for (i = 0; i < Size; i++)
    {
        if (flash[i] != ram[i])
            return LOADER_FAIL;
    }

    return LOADER_OK;
}
