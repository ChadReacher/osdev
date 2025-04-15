target remote localhost:1234
symbol-file build/kernel.elf
br kernel_start
layout src
continue
next
