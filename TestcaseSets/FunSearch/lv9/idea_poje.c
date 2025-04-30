/*
 * 测试用例：实现一个简单的计算器
 * 功能：用户输入两个整数和一个操作符，程序执行相应运算并输出结果。
 * 本测试用例旨在验证SysY语言中的函数定义与调用、逻辑运算、数组操作和变量shadow。
 */

// 加法函数
int add(int a, int b) {
    return a + b;
}

// 减法函数
int sub(int a, int b) {
    return a - b;
}

// 乘法函数
int mul(int a, int b) {
    return a * b;
}

// 除法函数
int div(int a, int b) {
    return a / b;
}

// 取模函数
int mod(int a, int b) {
    return a % b;
}

// 根据操作符选择运算
int calculate(int a, int b, int op) {
    if (op == 43) { // '+'
        return add(a, b);
    } else if (op == 45) { // '-'
        return sub(a, b);
    } else if (op == 42) { // '*'
        return mul(a, b);
    } else if (op == 47) { // '/'
        return div(a, b);
    } else if (op == 37) { // '%'
        return mod(a, b);
    } else {
        return 0;
    }
}

int main() {
    // 获取输入
    int a = getint();
    int b = getint();
    int op = getch();
    
    // 测试shadow变量
    {
        int a = 100;
        int b = 50;
        int op = 45; // '-'
        putint(calculate(a, b, op)); // 应输出50
        putch(10);
    }
    
    // 计算结果并输出
    int result = calculate(a, b, op);
    putint(result);
    putch(10);
    
    // 测试短路逻辑
    if (b != 0 || op != 47 || op != 37) {
        putch(69); // 'E'
        putch(110); // 'n'
        putch(100); // 'd'
    }
    
    return 0;
}