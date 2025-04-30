/*
 * 测试用例：实现一个简单的斐波那契数列计算器
 * 功能：用户输入一个整数n，程序计算并输出斐波那契数列的第n项
 * 本测试用例旨在验证递归函数、数组操作、逻辑运算和变量shadow
 */

// 使用递归计算斐波那契数列
int fibonacci_recursive(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci_recursive(n - 1) + fibonacci_recursive(n - 2);
}

// 使用迭代计算斐波那契数列
int fibonacci_iterative(int n) {
    if (n <= 1) {
        return n;
    }
    int fib[100];
    fib[0] = 0;
    fib[1] = 1;
    int i = 2;
    while (i <= n) {
        fib[i] = fib[i - 1] + fib[i - 2];
        i = i + 1;
    }
    return fib[n];
}

// 测试短路逻辑运算
void test_short_circuit(int n) {
    if (n < 0 || fibonacci_recursive(n) > 100) {
        putch(83); // 'S'
        putch(67); // 'C'
    }
}

int main() {
    // 获取用户输入
    int n = getint();
    
    // 测试shadow变量
    {
        int n = 5;
        putint(n); // 输出5
        putch(10);
    }
    
    // 计算并输出结果
    putch(82); // 'R'
    putch(58); // ':'
    putint(fibonacci_recursive(n));
    putch(10);
    
    putch(73); // 'I'
    putch(58); // ':'
    putint(fibonacci_iterative(n));
    putch(10);
    
    // 测试短路逻辑
    test_short_circuit(n);
    putch(10);
    
    return 0;
}