/*
 * 测试复杂逻辑运算、数组操作和函数调用的综合应用
 * 功能：模拟简单的计算器，支持加减乘除和取模运算
 */

// 全局常量定义
const int OP_ADD = 0;
const int OP_SUB = 1;
const int OP_MUL = 2;
const int OP_DIV = 3;
const int OP_MOD = 4;

// 计算两个数的运算结果
int calculate(int op, int a, int b) {
    if (op == OP_ADD) {
        return a + b;
    }
    if (op == OP_SUB) {
        return a - b;
    }
    if (op == OP_MUL) {
        return a * b;
    }
    if (op == OP_DIV) {
        return a / b;
    }
    if (op == OP_MOD) {
        return a % b;
    }
    return 0;
}

// 打印运算结果
void print_result(int op, int a, int b, int res) {
    putint(a);
    putch(32);
    if (op == OP_ADD) {
        putch(43); // '+'
    } else if (op == OP_SUB) {
        putch(45); // '-'
    } else if (op == OP_MUL) {
        putch(42); // '*'
    } else if (op == OP_DIV) {
        putch(47); // '/'
    } else {
        putch(37); // '%'
    }
    putch(32);
    putint(b);
    putch(32);
    putch(61); // '='
    putch(32);
    putint(res);
    putch(10);
}

int main() {
    // 测试各种运算
    int a = getint();
    int b = getint();
    
    // 测试所有运算符
    int op = 0;
    while (op <= 4) {
        // 测试除数为0的情况
        if (op == OP_DIV && b == 0) {
            putch(33); // '!'
            putch(10);
        } else if (op == OP_MOD && b == 0) {
            putch(33); // '!'
            putch(10);
        } else {
            int res = calculate(op, a, b);
            print_result(op, a, b, res);
        }
        op = op + 1;
    }
    
    // 测试数组和shadow
    {
        int arr[5] = {a, b, a+b, a-b, a*b};
        int i = 0;
        while (i < 5) {
            int a = arr[i]; // shadow外部a
            putint(a);
            putch(10);
            i = i + 1;
        }
    }
    
    return 0;
}