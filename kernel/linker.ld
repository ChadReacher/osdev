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

        _kernel_start_ = .;

	.text ALIGN(0x1000) : AT(ADDR(.text) - 0xC0000000)
	{
		*(.text);
	}

	.rodata ALIGN(0x1000) : AT(ADDR(.rodata) - 0xC0000000)
	{
		*(.rodata);
	}

	.data ALIGN(0x1000) : AT(ADDR(.data) - 0xC0000000)
	{
		*(.data);
	}

	.bss ALIGN(0x1000) : AT(ADDR(.bss) - 0xC0000000)
	{
		*(.bss);
	}

        _kernel_end_ = .;
}
