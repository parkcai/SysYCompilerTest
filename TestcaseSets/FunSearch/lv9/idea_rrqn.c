/*
 * 测试数组操作、逻辑运算短路、递归函数和变量shadow
 * 功能：实现一个简单的计算器，支持加法、减法、乘法和除法
 */

// 计算两个整数的和
int add(int a, int b) {
    return a + b;
}

// 计算两个整数的差
int subtract(int a, int b) {
    return a - b;
}

// 计算两个整数的积
int multiply(int a, int b) {
    return a * b;
}

// 计算两个整数的商（整数除法）
int divide(int a, int b) {
    if (b == 0) {
        putint(-1); // 除数为0，输出-1表示错误
        return 0;
    }
    return a / b;
}

// 打印计算结果
void print_result(int result) {
    putint(result);
    putch(10); // 换行
}

int main() {
    // 获取输入的两个整数
    int num1 = getint();
    int num2 = getint();

    // 测试变量shadow
    {
        int num1 = 5;
        int num2 = 3;
        print_result(add(num1, num2)); // 应输出8
    }

    // 计算并输出加法结果
    print_result(add(num1, num2));

    // 计算并输出减法结果
    print_result(subtract(num1, num2));

    // 计算并输出乘法结果
    print_result(multiply(num1, num2));

    // 计算并输出除法结果
    print_result(divide(num1, num2));

    return 0;
}