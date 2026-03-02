target remote localhost:1234
symbol-file build/kernel.elf
set disassembly-flavor intel
br kernel_start
layout src
continue
next
