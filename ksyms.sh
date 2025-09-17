#!/bin/sh
nm -n $1 > build/kernel.map
echo '#include <syms.h>' > ./syms.c
echo 'struct ksym ksymbols[] = {' >> ./syms.c
cat build/kernel.map | grep -i ' T ' | awk '{ printf("    { 0x%s, \"%s\" },\n", $1, $3); }' >> ./syms.c
echo '};' >> ./syms.c
echo 'u32 ksyms_count = sizeof(ksymbols) / sizeof(struct ksym);' >> ./syms.c
mv syms.c kernel/src/core/syms.c
