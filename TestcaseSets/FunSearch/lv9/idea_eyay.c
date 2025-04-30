/*
 * 测试多维数组、函数递归、逻辑运算和变量shadow
 * 功能：计算并输出斐波那契数列的前N项，并将结果存储在一个数组中
 */

// 计算斐波那契数列的第n项
int fibonacci(int n) {
    if (n <= 0) {
        return 0;
    } else if (n == 1) {
        return 1;
    } else {
        return fibonacci(n - 1) + fibonacci(n - 2);
    }
}

// 初始化斐波那契数列数组
void init_fibonacci_array(int fib[], int n) {
    int i = 0;
    while (i < n) {
        fib[i] = fibonacci(i + 1); // 计算并存储斐波那契数列的第i+1项
        i = i + 1;
    }
}

// 打印斐波那契数列数组
void print_fibonacci_array(int fib[], int n) {
    int i = 0;
    while (i < n) {
        putint(fib[i]);
        if (i != n - 1) {
            putch(32); // 空格
        }
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    int n = getint(); // 获取需要计算的斐波那契数列项数
    
    // 测试shadow变量
    {
        int n = 5;
        putint(n); // 应输出5而非输入值
        putch(10);
    }
    
    // 测试短路求值
    if (n <= 0 || n > 100) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        putch(10);
        return 0;
    }
    
    int fib[100];
    init_fibonacci_array(fib, n);
    print_fibonacci_array(fib, n);
    
    return 0;
}