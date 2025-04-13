# csmith

1. 可选参数见 options.txt， 不能完全符合文法，仍需手动修改
2. 让gpt生成了一个接近SysY的option：
    csmith --seed 12345 --output random.c --no-longlong --no-float --no-int8 --no-uint8 --no-math64 --no-builtins --arrays --pointers --no-volatile-pointers --no-const-pointers --no-bitfields --no-comma-operators --no-compound-assignment --no-embedded-assigns --no-pre-incr-operator --no-pre-decr-operator --no-post-incr-operator --no-post-decr-operator --no-unary-plus-operator --no-jumps --main --no-inline-function --no-structs --no-unions --no-volatiles --no-global-variables --no-paranoid --no-safe-math --no-packed-struct
    代码见random.c
3. 可以截取其中某个函数，手动修改至符合文法后使用
