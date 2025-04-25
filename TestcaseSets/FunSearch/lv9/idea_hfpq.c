/*
 * 测试数组操作、函数递归、短路求值和变量shadow
 * 功能：实现一个简单的计算器，支持加减乘除和阶乘运算
 */

// 计算阶乘
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

// 计算表达式值
int calculate(int a, int b, int op) {
    if (op == 1) return a + b;
    if (op == 2) return a - b;
    if (op == 3) return a * b;
    if (op == 4) return a / b;
    return 0;
}

int main() {
    // 测试变量shadow
    int a = 10;
    {
        int a = 20;
        putint(a); // 应输出20
        putch(10);
    }
    
    // 获取操作类型
    putch(79); // 'O'
    putch(112); // 'p'
    putch(58); // ':'
    putch(32); // ' '
    int op = getint();
    
    // 测试短路求值
    if (op < 1 || op > 5) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        return 0;
    }
    
    // 处理阶乘运算
    if (op == 5) {
        putch(78); // 'N'
        putch(58); // ':'
        putch(32); // ' '
        int n = getint();
        putint(factorial(n));
        return 0;
    }
    
    // 处理二元运算
    putch(65); // 'A'
    putch(58); // ':'
    putch(32); // ' '
    int a_val = getint();
    putch(66); // 'B'
    putch(58); // ':'
    putch(32); // ' '
    int b_val = getint();
    
    // 计算并输出结果
    putint(calculate(a_val, b_val, op));
    return 0;
}