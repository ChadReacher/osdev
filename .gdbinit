target remote localhost:1234
symbol-file build/kernel.elf
br _start
layout src
continue
next
