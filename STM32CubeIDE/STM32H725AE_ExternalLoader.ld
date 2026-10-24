/*
 * STM32H725 external-loader linker script for STM32CubeProgrammer.
 *
 * Important CubeProgrammer/H7 layout:
 *
 *   0x24000000        reserved by CubeProgrammer for BKPT return stub
 *   0x24000004        first byte of external-loader ELF LOAD image
 *   0x24000200        vector table (0x200 aligned)
 *
 * This follows the established STM32H7 external-loader GCC layout.
 */

ENTRY(Init)

PHDRS
{
    Loader PT_LOAD;
    SgInfo PT_LOAD;
}

MEMORY
{
    /*
     * AXI SRAM starts at 0x24000000.
     * First four bytes are reserved by CubeProgrammer.
     * End remains 0x24050000 for the 320 KiB region used here.
     */
    RAM (xrw) : ORIGIN = 0x24000004, LENGTH = 0x0004FFFC
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

_Min_Heap_Size  = 0x200;
_Min_Stack_Size = 0x800;

SECTIONS
{
    /*
     * Start at 0x24000004, reserve 0x1FC bytes:
     *
     *   0x24000004 + 0x1FC = 0x24000200
     *
     * Cortex-M7 VTOR requires appropriate alignment and the H7 external
     * loader convention places the vector table here.
     */
    .isr_vector :
    {
        . = . + 0x1FC;
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >RAM :Loader

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } >RAM :Loader

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } >RAM :Loader

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } >RAM :Loader

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } >RAM :Loader

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } >RAM :Loader

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } >RAM :Loader

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } >RAM :Loader

    .text :
    {
        . = ALIGN(4);

        KEEP(*(.text.Init))
        KEEP(*(.text.Read))
        KEEP(*(.text.Write))
        KEEP(*(.text.SectorErase))
        KEEP(*(.text.MassErase))
        KEEP(*(.text.CheckSum))
        KEEP(*(.text.Verify))

        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)

        KEEP(*(.init))
        KEEP(*(.fini))

        . = ALIGN(4);
        _etext = .;
    } >RAM :Loader

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } >RAM :Loader

    ._user_heap_stack (NOLOAD) :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } >RAM :Loader

    /*
     * STM32CubeProgrammer metadata in a separate LOAD program header.
     */
    .Dev_info :
    {
        KEEP(*(.Dev_info))
    } >RAM :SgInfo

    .ARM.attributes 0 :
    {
        *(.ARM.attributes)
    }

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
    }
}
