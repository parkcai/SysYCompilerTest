# ASSESSMENT & EVALUATOR  

from zqc

## LLVM工具的手动Usage  

1. 挂docker:   docker run -it --rm -v D:\for_GIT\Sysy\SysYCompilerTest:/root/SysYCompilerTest maxxing/compiler-dev bash  
2. cd SysYCompilerTest/Compilers/Compiler2/lv9
3. 用Workingtable中的makefile替换SysYCompilerTest/Compilers/Compiler2/lv9/Makefile
4. make clean
5. make COVERAGE=1
6. LLVM_PROFILE_FILE="test1.profraw" ./build/compiler -riscv /root/SysYCompilerTest/TestcaseSets/CourseOriginal/lv8/00_int_func.c  -o test1.riscv.S
7. LLVM_PROFILE_FILE="test2.profraw" ./build/compiler -riscv /root/SysYCompilerTest/TestcaseSets/CourseOriginal/lv8/06_complex_call.c  -o test2.riscv.S
8. llvm-profdata-13 merge -sparse -o default.profdata test1.profraw test2.profraw
9. llvm-cov report ./build/compiler -instr-profile=default.profdata > score.txt
10. rm -f score.txt *.profdata *.profraw *.riscv.S
11. //不要make clean, 重新开始./build/compiler 就行  

## ASSESSMENT.py & compiler_coverage_score  

1. 运行函数需要你手动：1.挂载镜像 2.替换makefile 3.make clean 4.make COVERAGE=1
2. 参数为 1.可执行的编译器文件路径 2. 包含c文件的文件夹路径
3. 返回百分制.2f 是块覆盖和分支覆盖的平均值 #可根据爱好自行修改

## EVALUATOR

1. 可以通过assessment.compiler_coverage_score简单实现
2. 由于一些环境问题，我们可以在讨论后编写
