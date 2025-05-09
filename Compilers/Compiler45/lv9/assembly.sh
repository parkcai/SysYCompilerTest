#!/usr/bin/bash

clang temp.S -c -o temp.o -target riscv32-unknown-linux-elf -march=rv32im -mabi=ilp32
ld.lld temp.o -L"$CDE_LIBRARY_PATH"/riscv32 -lsysy -o temp
qemu-riscv32-static temp
echo $?
