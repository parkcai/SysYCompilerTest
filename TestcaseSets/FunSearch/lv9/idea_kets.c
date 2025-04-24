/*
 * 测试函数递归、数组操作和逻辑运算
 * 功能：计算并输出斐波那契数列前n项，同时测试变量shadow
 */

// 递归计算斐波那契数列
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 打印数组元素
void print_array(int arr[], int size) {
    int i = 0;
    while (i < size) {
        putint(arr[i]);
        putch(32); // 空格
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    int n = getint(); // 获取数列项数
    
    // 测试短路求值
    if (n <= 0 || n > 15) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        return 0;
    }
    
    // 生成斐波那契数列
    int fib[15];
    int i = 0;
    while (i < n) {
        fib[i] = fibonacci(i);
        i = i + 1;
    }
    
    // 测试shadow变量
    {
        int i = 999;
        putint(i); // 应输出999
        putch(10); // 换行
    }
    
    // 输出结果
    print_array(fib, n);
    
    // 测试数组访问
    putint(fib[n-1]); // 输出最后一项
    return 0;
}