/* 计算斐波那契数列并输出的SysY程序 */
int fib(int n) {  // 递归实现斐波那契函数
    if (n <= 1) {
        return n;
    }
    return fib(n-1) + fib(n-2);
}

int main() {
    int arr[10];
    const int size = 10;  // 正确使用编译期常量
    
    /* 初始化数组 */
    int i = 0;
    while (i < size) {
        arr[i] = fib(i);
        i = i + 1;
    }
    
    {
        // 测试变量shadow
        int i = 5;
        putint(arr[i]);  // 输出第6个斐波那契数（应为5）
        putch(10);       // 输出换行符
    }
    
    // 输出整个数组
    putarray(size, arr);
    return 0;
}