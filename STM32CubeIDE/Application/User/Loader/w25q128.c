#include "w25q128.h"
#include "ospi2_board.h"
#include "loader_diag.h"

/* W25Q128JV commands, standard SPI / 1-1-1 mode */
#define CMD_WRITE_ENABLE         0x06U
#define CMD_READ_STATUS1         0x05U
#define CMD_READ_DATA            0x03U
#define CMD_PAGE_PROGRAM         0x02U
#define CMD_SECTOR_ERASE_4K      0x20U
#define CMD_CHIP_ERASE           0xC7U
#define CMD_JEDEC_ID             0x9FU
#define CMD_ENABLE_RESET         0x66U
#define CMD_RESET_DEVICE         0x99U

#define SR1_BUSY                 0x01U
#define W25_TIMEOUT_CMD_MS       100U
#define W25_TIMEOUT_PROGRAM_MS   1000U
#define W25_TIMEOUT_ERASE_MS     5000U
#define W25_TIMEOUT_CHIP_MS      120000U

static int cmd_only(uint8_t instruction)
{
    OSPI_RegularCmdTypeDef cmd = {0};

    cmd.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
    cmd.FlashId = HAL_OSPI_FLASH_ID_1;
    cmd.Instruction = instruction;
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    cmd.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
    cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    cmd.AddressMode = HAL_OSPI_ADDRESS_NONE;
    cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = HAL_OSPI_DATA_NONE;
    cmd.DummyCycles = 0;
    cmd.DQSMode = HAL_OSPI_DQS_DISABLE;
    cmd.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    return (HAL_OSPI_Command(&hospi2, &cmd, W25_TIMEOUT_CMD_MS) == HAL_OK)
        ? W25Q128_OK : W25Q128_ERROR;
}

static int write_enable(void)
{
    if (cmd_only(CMD_WRITE_ENABLE) != W25Q128_OK)
        return W25Q128_ERROR;
    return W25Q128_OK;
}

static int read_status1(uint8_t *sr)
{
    OSPI_RegularCmdTypeDef cmd = {0};

    cmd.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
    cmd.FlashId = HAL_OSPI_FLASH_ID_1;
    cmd.Instruction = CMD_READ_STATUS1;
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    cmd.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
    cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    cmd.AddressMode = HAL_OSPI_ADDRESS_NONE;
    cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = HAL_OSPI_DATA_1_LINE;
    cmd.NbData = 1;
    cmd.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
    cmd.DummyCycles = 0;
    cmd.DQSMode = HAL_OSPI_DQS_DISABLE;
    cmd.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(&hospi2, &cmd, W25_TIMEOUT_CMD_MS) != HAL_OK)
        return W25Q128_ERROR;

    if (HAL_OSPI_Receive(&hospi2, sr, W25_TIMEOUT_CMD_MS) != HAL_OK)
    {
        LoaderDiag_Sub(0x0000E221U);
        LoaderDiag_OSPI(&hospi2);
        return W25Q128_ERROR;
    }

    LoaderDiag_Status1(*sr);
    return W25Q128_OK;
}

int W25Q128_WaitReady(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    uint8_t sr;

    do
    {
        if (read_status1(&sr) != W25Q128_OK)
            return W25Q128_ERROR;

        if ((sr & SR1_BUSY) == 0U)
            return W25Q128_OK;

    } while ((HAL_GetTick() - start) < timeout_ms);

    return W25Q128_ERROR;
}

int W25Q128_ReadJedecId(uint8_t id[3])
{
    OSPI_RegularCmdTypeDef cmd = {0};

    cmd.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
    cmd.FlashId = HAL_OSPI_FLASH_ID_1;
    cmd.Instruction = CMD_JEDEC_ID;
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    cmd.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
    cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    cmd.AddressMode = HAL_OSPI_ADDRESS_NONE;
    cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = HAL_OSPI_DATA_1_LINE;
    cmd.NbData = 3;
    cmd.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
    cmd.DummyCycles = 0;
    cmd.DQSMode = HAL_OSPI_DQS_DISABLE;
    cmd.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(&hospi2, &cmd, W25_TIMEOUT_CMD_MS) != HAL_OK)
        return W25Q128_ERROR;

    if (HAL_OSPI_Receive(&hospi2, id, W25_TIMEOUT_CMD_MS) != HAL_OK)
        return W25Q128_ERROR;

    return W25Q128_OK;
}

int W25Q128_Init(void)
{
    uint8_t id[3] = {0, 0, 0};

    LoaderDiag_Sub(0x00000200U);

    /*
     * Try reset commands but preserve the original behavior: a reset-command
     * failure alone does not abort Init, because the flash may already be in
     * ordinary SPI state.
     */
    LoaderDiag_Sub(0x00000210U);
    (void)cmd_only(CMD_ENABLE_RESET);

    LoaderDiag_Sub(0x00000220U);
    (void)cmd_only(CMD_RESET_DEVICE);

    HAL_Delay(1);

    LoaderDiag_Sub(0x00000230U); /* wait ready */
    if (W25Q128_WaitReady(100U) != W25Q128_OK)
    {
        LoaderDiag_Sub(0x0000E230U);
        LoaderDiag_OSPI(&hospi2);
        return W25Q128_ERROR;
    }

    LoaderDiag_Sub(0x00000240U); /* JEDEC read */
    if (W25Q128_ReadJedecId(id) != W25Q128_OK)
    {
        LoaderDiag_Sub(0x0000E240U);
        LoaderDiag_OSPI(&hospi2);
        return W25Q128_ERROR;
    }

    LoaderDiag_Jedec(id);

    if (id[0] != 0xEFU || id[1] != 0x40U || id[2] != 0x18U)
    {
        LoaderDiag_Sub(0x0000E250U); /* JEDEC mismatch */
        LoaderDiag_OSPI(&hospi2);
        LoaderDiag_OSPIRegs();
        return W25Q128_ERROR;
    }

    LoaderDiag_Sub(0x00000260U); /* W25 init complete */
    return W25Q128_OK;
}

int W25Q128_Read(uint32_t address, uint8_t *data, uint32_t size)
{
    OSPI_RegularCmdTypeDef cmd = {0};

    if (size == 0U)
        return W25Q128_OK;

    cmd.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
    cmd.FlashId = HAL_OSPI_FLASH_ID_1;
    cmd.Instruction = CMD_READ_DATA;
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    cmd.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
    cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    cmd.Address = address & 0x00FFFFFFUL;
    cmd.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
    cmd.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
    cmd.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;
    cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = HAL_OSPI_DATA_1_LINE;
    cmd.NbData = size;
    cmd.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
    cmd.DummyCycles = 0;
    cmd.DQSMode = HAL_OSPI_DQS_DISABLE;
    cmd.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(&hospi2, &cmd, W25_TIMEOUT_CMD_MS) != HAL_OK)
        return W25Q128_ERROR;

    if (HAL_OSPI_Receive(&hospi2, data, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return W25Q128_ERROR;

    return W25Q128_OK;
}

int W25Q128_PageProgram(uint32_t address, const uint8_t *data, uint32_t size)
{
    OSPI_RegularCmdTypeDef cmd = {0};

    if (size == 0U || size > 256U)
        return W25Q128_ERROR;

    if (((address & 0xFFU) + size) > 256U)
        return W25Q128_ERROR;

    if (write_enable() != W25Q128_OK)
        return W25Q128_ERROR;

    cmd.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
    cmd.FlashId = HAL_OSPI_FLASH_ID_1;
    cmd.Instruction = CMD_PAGE_PROGRAM;
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    cmd.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
    cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    cmd.Address = address & 0x00FFFFFFUL;
    cmd.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
    cmd.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
    cmd.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;
    cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = HAL_OSPI_DATA_1_LINE;
    cmd.NbData = size;
    cmd.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
    cmd.DummyCycles = 0;
    cmd.DQSMode = HAL_OSPI_DQS_DISABLE;
    cmd.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(&hospi2, &cmd, W25_TIMEOUT_CMD_MS) != HAL_OK)
        return W25Q128_ERROR;

    if (HAL_OSPI_Transmit(&hospi2, (uint8_t *)data, W25_TIMEOUT_PROGRAM_MS) != HAL_OK)
        return W25Q128_ERROR;

    return W25Q128_WaitReady(W25_TIMEOUT_PROGRAM_MS);
}

int W25Q128_SectorErase4K(uint32_t address)
{
    OSPI_RegularCmdTypeDef cmd = {0};

    if (write_enable() != W25Q128_OK)
        return W25Q128_ERROR;

    cmd.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
    cmd.FlashId = HAL_OSPI_FLASH_ID_1;
    cmd.Instruction = CMD_SECTOR_ERASE_4K;
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    cmd.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
    cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    cmd.Address = address & 0x00FFF000UL;
    cmd.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
    cmd.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
    cmd.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;
    cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = HAL_OSPI_DATA_NONE;
    cmd.DummyCycles = 0;
    cmd.DQSMode = HAL_OSPI_DQS_DISABLE;
    cmd.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(&hospi2, &cmd, W25_TIMEOUT_CMD_MS) != HAL_OK)
        return W25Q128_ERROR;

    return W25Q128_WaitReady(W25_TIMEOUT_ERASE_MS);
}

int W25Q128_ChipErase(void)
{
    if (write_enable() != W25Q128_OK)
        return W25Q128_ERROR;

    if (cmd_only(CMD_CHIP_ERASE) != W25Q128_OK)
        return W25Q128_ERROR;

    return W25Q128_WaitReady(W25_TIMEOUT_CHIP_MS);
}
int W25Q128_DisableMemoryMappedMode(void)
{
    /*
     * HAL_OSPI_Abort() exits memory-mapped mode and returns OCTOSPI2
     * to indirect-command operation.
     */
    if (HAL_OSPI_Abort(&hospi2) != HAL_OK)
        return W25Q128_ERROR;

    return W25Q128_OK;
}

int W25Q128_EnableMemoryMappedMode(void)
{
    OSPI_RegularCmdTypeDef cmd = {0};
    OSPI_MemoryMappedTypeDef mm = {0};

    /*
     * STM32H7 HAL requires BOTH read and write memory-mapped command
     * configurations before HAL_OSPI_MemoryMapped().
     */

    /* READ configuration: 0x03 + 24-bit address + 1-line data */
    cmd.OperationType = HAL_OSPI_OPTYPE_READ_CFG;
    cmd.FlashId = HAL_OSPI_FLASH_ID_1;

    cmd.Instruction = CMD_READ_DATA;
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    cmd.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
    cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;

    cmd.Address = 0;
    cmd.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
    cmd.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
    cmd.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;

    cmd.AlternateBytes = 0;
    cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    cmd.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_8_BITS;
    cmd.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE;

    cmd.DataMode = HAL_OSPI_DATA_1_LINE;
    cmd.NbData = 0; /* ignored for memory-mapped configuration */
    cmd.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;

    cmd.DummyCycles = 0;
    cmd.DQSMode = HAL_OSPI_DQS_DISABLE;
    cmd.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    LoaderDiag_Sub(0x00000310U); /* before READ_CFG */
    if (HAL_OSPI_Command(&hospi2,
                         &cmd,
                         HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        LoaderDiag_Sub(0x0000E310U);
        LoaderDiag_OSPI(&hospi2);
        return W25Q128_ERROR;
    }
    LoaderDiag_OSPI(&hospi2);

    /*
     * READ_CFG leaves the H7 HAL in HAL_OSPI_STATE_READ_CMD_CFG.
     * Adding WRITE_CFG transitions it to HAL_OSPI_STATE_CMD_CFG,
     * which HAL_OSPI_MemoryMapped() requires.
     */
    cmd.OperationType = HAL_OSPI_OPTYPE_WRITE_CFG;
    cmd.Instruction = CMD_PAGE_PROGRAM;
    cmd.DummyCycles = 0;

    LoaderDiag_Sub(0x00000320U); /* before WRITE_CFG */
    if (HAL_OSPI_Command(&hospi2,
                         &cmd,
                         HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        LoaderDiag_Sub(0x0000E320U);
        LoaderDiag_OSPI(&hospi2);
        return W25Q128_ERROR;
    }
    LoaderDiag_OSPI(&hospi2);

    mm.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_DISABLE;
    mm.TimeOutPeriod = 0;

    LoaderDiag_Sub(0x00000330U); /* before HAL_OSPI_MemoryMapped */
    if (HAL_OSPI_MemoryMapped(&hospi2, &mm) != HAL_OK)
    {
        LoaderDiag_Sub(0x0000E330U);
        LoaderDiag_OSPI(&hospi2);
        return W25Q128_ERROR;
    }

    LoaderDiag_Sub(0x00000340U); /* memory-mapped active */
    LoaderDiag_OSPI(&hospi2);
    return W25Q128_OK;
}
