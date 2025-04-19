# PKU 软件测试导论 2025 春作业

## 项目说明

### 下载项目与配置环境

- 运行 git clone https://github.com/parkcai/SysYCompilerTest 以下载项目至目标文件夹

- 运行 docker pull maxxing/compiler-dev 以下载【北大编译实践教学用编译器开发环境】（详见[北大编译实践在线文档](https://pku-minic.github.io/online-doc/)）

- 运行 docker run -it --rm -v /path/to/your/downloaded/program:/root/SysYCompilerTest maxxing/compiler-dev bash 以创建上述环境的临时容器并挂载项目目录；请自行替换 /path/to/your/downloaded/program ，如在 Windows 系统上可能替换成：docker run -it --rm -v D:\MyGithubPrograms\SysYCompilerTest:/root/SysYCompilerTest maxxing/compiler-dev bash

- CompilerTester 中的 test_compiler.py 封装了三个函数：test_compiler_with_testcase_set, use_compiler, run_riscv_asm，可以自由在 main.py 中使用；使用方法：在容器内部运行 python3 /root/SysYCompilerTest/CompilerTester/main.py

## 研究目的

114514

## 研究方法

1919810

## 研究结论

666,666

## References

[1] 北大编译实践在线文档 https://pku-minic.github.io/online-doc/

[2] 北大编译实践教学用编译器开发环境 https://github.com/pku-minic/compiler-dev/tree/master

[3] 本人作业（Compiler1） https://gitlab.eduxiji.net/pku2200011363/compiler-of-parkcai

[4] vvvvsv 作业（Compiler2） https://github.com/vvvvsv/PKU-SysY-Compiler

[5] GeorgeMLP 作业（Compiler3） https://github.com/GeorgeMLP/sysy-compiler

[6] ZhaoChunshan 作业（Compiler4） https://github.com/ZhaoChunshan/SysyCompiler

[7] CaptainChen 作业（Compiler5） https://gitee.com/CaptainChen/stupid-sys-y2-rv

[8] NovaPigeon 作业（Compiler6） https://github.com/NovaPigeon/sysy-compiler
