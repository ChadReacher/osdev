#!/bin/bash

set -ex

export CC=/usr/bin/gcc
export LD=/usr/bin/ld
export PREFIX="/usr/bin/i386elfgcc"
export TARGET=i386-elf
export PATH="$PREFIX/bin:$PATH"


sudo apt install build-essential libgmp3-dev libmpc-dev libmpfr-dev texinfo
mkdir /tmp/toolchain

cd /tmp/toolchain
curl -O https://ftp.gnu.org/gnu/binutils/binutils-2.41.tar.xz
tar xf binutils-2.41.tar.xz
mkdir binutils-build
cd binutils-build
../binutils-2.41/configure --target="$TARGET" --prefix="$PREFIX" --enable-interwork --enable-multilib --disable-nls --disable-werror 2>&1 | tee configure.log
make all install 2>&1 | tee make.log

cd /tmp/toolchain
curl -O https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.gz
tar xf gcc-13.2.0.tar.gz
mkdir gcc-build
cd gcc-build
../gcc-13.2.0/configure --target="$TARGET" --prefix="$PREFIX" --disable-nls --disable-libssp --enable-languages=c,c++ --without-headers
make -j 2 all-gcc 
make -j 2 all-target-libgcc 
make -j 2 install-gcc 
make -j 2 install-target-libgcc 
