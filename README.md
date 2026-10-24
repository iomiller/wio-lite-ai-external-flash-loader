# Wio Lite AI board's serial flash chip (25Q128JVSIQ) external memory loader for STM32CubeProgrammer.

An STM32CubeProgrammer / STM32CubeIDE external flash loader (`.stldr`) for the 16 MiB Winbond W25Q128JVSIQ QSPI flash on the Seeed Wio Lite AI board. The flash is connected to OCTOSPI2.

- Flash: 16 MiB at `0x70000000`. Pages are 256 B and sectors are 4 KiB (4096 sectors).
- Interface: OCTOSPIM port 2, high nibble (IO4–IO7, CLK on PF4, NCS on PG12). It currently uses 1-1-1 SPI mode only.
- Device name: `W25Q128JVSIQ_STM32H725AE_OSPI2`

## Build

Open the project in STM32CubeIDE and build the **Release** configuration. The build outputs `STM32CubeIDE/Release/W25Q128JVSIQ_STM32H725AEI6_WIO_LITE_AI.elf`.

(The preserved `.stldr` build artifact is located in the project root directory)

## Install

Rename the `.elf` file to `.stldr` so STM32CubeProgrammer recognizes it. Copy it into STM32CubeProgrammer's `bin/ExternalLoader/` folder, then select it under **External loaders**.

Loader sources are in `STM32CubeIDE/Application/User/Loader/`.
