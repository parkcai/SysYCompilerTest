/*
 * 测试多维数组、递归函数、逻辑运算短路和变量shadow
 * 功能：计算并输出阶乘，并验证输入的有效性
 */

// 递归计算阶乘
int factorial(int n) {
    if (n == 0 || n == 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

// 验证输入是否为非负整数
void validate_input(int n) {
    if (n < 0) {
        putch(33); // 输出'!'表示无效输入
        return;
    }
    putint(factorial(n));
    putch(10); // 换行
}

int main() {
    int n = getint(); // 获取输入的整数
    
    // 测试变量shadow
    {
        int n = 5; // shadow外层n
        putint(n); // 应输出5
        putch(10);
    }
    
    // 测试短路特性
    if (n >= 0 && n <= 12) {
        validate_input(n);
    } else {
        putch(33); // 输出'!'表示无效输入
    }
    
    return 0;
}