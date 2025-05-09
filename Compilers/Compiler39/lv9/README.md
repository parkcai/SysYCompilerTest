
docker run -it --rm -v C:/Users/15165/compiler/sysy-make-template:/root/compiler maxxing/compiler-dev /bin/bash
./build/compiler -riscv ./debug/hello.c -o ./hello.S
./build/compiler -koopa ./debug/hello.c -o ./hello.koopa
git config --global user.email "1516505913@qq.com"
git config --global user.name "zly"
glpat-sxXMbutNd2LDLY1bigGH
FUStWQX5

koopac hello.koopa | llc --filetype=obj -o hello.o
clang hello.o -L$CDE_LIBRARY_PATH/native -lsysy -o hello
./hello