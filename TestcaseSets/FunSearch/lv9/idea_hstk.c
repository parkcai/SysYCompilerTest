/*
 * 测试数组初始化、函数递归、逻辑运算和变量shadow
 * 功能：计算并输出斐波那契数列前N项
 */

// 递归计算斐波那契数列
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 初始化数组为斐波那契数列
void init_fib_array(int arr[], int size) {
    int i = 0;
    while (i < size) {
        arr[i] = fibonacci(i);
        i = i + 1;
    }
}

int main() {
    int n = getint(); // 获取斐波那契数列项数
    int fib[20];      // 存储斐波那契数列
    
    // 测试输入合法性
    if (n <= 0 || n > 20) {
        putch(33); // 输出'!'表示错误
        return -1;
    }
    
    // 初始化数组
    init_fib_array(fib, n);
    
    // 输出结果
    putarray(n, fib);
    
    // 测试shadow变量和逻辑运算
    {
        int n = 5;
        if (n > 0 && (fib[n-1] > 0)) {
            putch(10); // 换行
            putint(fib[n-1]); // 应输出fib[4]=3
        }
    }
    
    return 0;
}