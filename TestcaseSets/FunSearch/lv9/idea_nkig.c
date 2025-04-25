/*
 * 测试复杂表达式、数组操作和函数递归
 * 功能：计算并输出斐波那契数列，同时测试短路求值和变量shadow
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
        if (i < size - 1) {
            putch(32); // 空格
        }
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    int n = getint(); // 获取斐波那契数列长度
    
    // 测试短路求值
    if (n <= 0 || n > 20) {
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
    int fib[20];
    int i = 0;
    while (i < n) {
        fib[i] = fibonacci(i);
        i = i + 1;
    }
    
    // 输出结果
    print_array(fib, n);
    
    // 测试shadow变量
    {
        int n = 5;
        int fib[5] = {1, 1, 2, 3, 5};
        print_array(fib, n); // 应输出1 1 2 3 5
    }
    
    // 测试复杂表达式
    putint((fibonacci(10) + 5) * 2 - 3);
    return 0;
}