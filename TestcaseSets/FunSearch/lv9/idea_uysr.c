/* 计算斐波那契数列并输出前n项 */
int fib(int n) {  // 递归实现斐波那契
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

int main() {
    int n = getint();  // 获取输入项数
    int arr[10];
    int i = 0;
    
    /* 生成数列并处理数组越界 */
    while (i < n && i < 10) {
        arr[i] = fib(i);
        if (i % 2 == 0 || (i > 5 && i < 8)) {
            putint(arr[i]);  // 输出特定条件项
            putch(32);       // 输出空格
        }
        i = i + 1;
    }
    
    // 测试作用域shadow
    {
        int i = 99;
        putch(10);    // 换行
        putint(i);    // 应输出99
    }
    
    return 0;
}