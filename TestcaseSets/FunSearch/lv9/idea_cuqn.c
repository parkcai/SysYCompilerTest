/*
 * 测试函数递归、数组操作、短路求值和变量shadow
 * 功能：模拟简单的计算器，支持加减乘除和模运算，并测试嵌套作用域
 */

// 递归计算阶乘
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

// 计算两个数的运算结果
int compute(int a, int b, int op) {
    if (op == 0) return a + b;
    if (op == 1) return a - b;
    if (op == 2) return a * b;
    if (op == 3 && b != 0) return a / b;  // 测试短路求值
    if (op == 4 && b != 0) return a % b;  // 测试短路求值
    return -1;
}

// 打印运算结果
void print_result(int res) {
    if (res == -1) {
        putch(69); // 'E'
        putch(114); // 'r'
        putch(114); // 'r'
        putch(111); // 'o'
        putch(114); // 'r'
    } else {
        putint(res);
    }
    putch(10); // 换行
}

int main() {
    // 获取输入
    int a = getint();
    int b = getint();
    int op = getint();
    
    // 测试shadow
    {
        int a = 10;
        int b = 2;
        int op = 2;
        int res = compute(a, b, op);
        print_result(res); // 应输出20
    }
    
    // 计算并输出结果
    int res = compute(a, b, op);
    print_result(res);
    
    // 测试数组和递归
    int arr[5];
    int i = 0;
    while (i < 5) {
        arr[i] = factorial(i + 1);
        i = i + 1;
    }
    
    // 输出数组
    putarray(5, arr);
    
    return 0;
}