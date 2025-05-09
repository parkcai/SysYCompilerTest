#!/usr/bin/bash

for i in {1..500}; do
    echo "Test $i:"
    ./target/release/compiler -riscv temp.c -o temp.S
    if [ $? -ne 0 ]; then
        echo "Failed to compile"
        exit 1
    fi
    clang temp.S -c -o temp.o -target riscv32-unknown-linux-elf -march=rv32im -mabi=ilp32
    ld.lld temp.o -L"$CDE_LIBRARY_PATH"/riscv32 -lsysy -o temp
    qemu-riscv32-static temp
    a=$?
    #    echo "Got: $a"

    clang temp.c -o temp -w
    ./temp
    b=$?
    #    echo "Expect: $b"
    if [ $a -ne $b ]; then
        echo "Mismatch: $a $b"
        exit 1
    fi
done
