/*
 * 测试多维数组、函数递归、逻辑运算和变量shadow
 * 功能：实现一个简单的计算器，支持加减乘除四则运算
 */

// 定义操作符枚举
const int ADD = 1;
const int SUB = 2;
const int MUL = 3;
const int DIV = 4;

// 计算两个整数的四则运算
int calculate(int a, int b, int op) {
    if (op == ADD) return a + b;
    else if (op == SUB) return a - b;
    else if (op == MUL) return a * b;
    else if (op == DIV) {
        if (b != 0) return a / b;
        else return -1; // 除数为0时返回-1
    }
    return -1; // 操作符不合法时返回-1
}

// 打印结果
void print_result(int result) {
    if (result == -1) {
        putch(78); // 'N'
        putch(111); // 'o'
        putch(116); // 't'
        putch(32); // 空格
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        putch(10); // 换行
    } else {
        putint(result);
        putch(10); // 换行
    }
}

int main() {
    // 获取输入
    int a = getint();
    int b = getint();
    int op = getint();
    
    // 测试逻辑运算
    if (a < 0 || b < 0 || (op != ADD && op != SUB && op != MUL && op != DIV)) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'd'
        putch(32); // 空格
        putch(105); // 'i'
        putch(110); // 'n'
        putch(112); // 'p'
        putch(117); // 'u'
        putch(116); // 't'
        putch(10); // 换行
        return 0;
    }
    
    // 计算并打印结果
    int result = calculate(a, b, op);
    print_result(result);
    
    // 测试shadow变量
    {
        int a = 999;
        putint(a);
        putch(10);
    }
    
    return 0;
}