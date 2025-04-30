/*
 * 测试多维数组、函数递归、逻辑运算短路和变量shadow
 * 功能：实现一个简单的计算器，支持加法、减法、乘法和除法
 */

// 定义常量
const int ADD = 0;
const int SUB = 1;
const int MUL = 2;
const int DIV = 3;

// 计算器函数
int calculate(int a, int b, int op) {
    if (op == ADD) {
        return a + b;
    } else if (op == SUB) {
        return a - b;
    } else if (op == MUL) {
        return a * b;
    } else if (op == DIV) {
        if (b != 0) {
            return a / b;
        } else {
            putch(33); // 输出'!'表示错误
            return 0; // 返回0表示除数为0的错误
        }
    } else {
        putch(33); // 输出'!'表示错误
        return 0; // 返回0表示操作符错误
    }
}

// 打印结果
void print_result(int result) {
    putint(result);
    putch(10); // 换行
}

int main() {
    // 读取输入
    int a = getint();
    int b = getint();
    int op = getint();

    // 测试shadow变量
    {
        int a = 100;
        putint(a); // 应输出100
        putch(10);
    }

    // 计算并打印结果
    int result = calculate(a, b, op);
    print_result(result);

    return 0;
}