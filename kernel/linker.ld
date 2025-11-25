ENTRY(_start)
OUTPUT_ARCH(i386)
OUTPUT_FORMAT("elf32-i386")

SECTIONS
{
    . = 0x100000;

    .setup ALIGN(0x1000):
    {
        *(.setup);
    }

    . += 0xC0000000;

    .text ALIGN(0x1000) : AT(ADDR(.text) - 0xC0000000)
    {
        _text_start_ = .;
        *(.text);
        _text_end_ = .;
    }

    .rodata ALIGN(0x1000) : AT(ADDR(.rodata) - 0xC0000000)
    {
        _rodata_start_ = .;
        *(.rodata);
        _rodata_end_ = .;
    }

    .data ALIGN(0x1000) : AT(ADDR(.data) - 0xC0000000)
    {
        _data_start_ = .;
        *(.data);
        _data_end_ = .;
    }

    .bss ALIGN(0x1000) : AT(ADDR(.bss) - 0xC0000000)
    {
        _bss_start_ = .;
        *(.bss);
        _bss_end_ = .;
    }
}
