#!/usr/bin/bash

./target/release/compiler -koopa temp.c -o temp.koopa
koopac temp.koopa | llc --filetype=obj -o temp.o
clang temp.o -L"$CDE_LIBRARY_PATH"/native -lsysy -o temp
./temp
echo $?

clang temp.c -o temp
./temp
echo $?
