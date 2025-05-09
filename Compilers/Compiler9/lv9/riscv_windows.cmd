cargo run -- -riscv hello.c -o hello.S
gcc hello.c -o hello.exe
hello.exe
echo %ERRORLEVEL%