/*
 * 测试用例：实现一个简单的素数判断器
 * 功能：用户输入一个整数，程序判断该整数是否为素数，并输出相应的结果。
 * 本测试用例旨在验证SysY语言中的函数定义与调用、逻辑运算和数组操作。
 */

// 判断一个数是否为素数
int is_prime(int n) {
    if (n <= 1) {
        return 0; // 非素数
    }
    int i = 2;
    while (i * i <= n) {
        if (n % i == 0) {
            return 0; // 非素数
        }
        i = i + 1;
    }
    return 1; // 素数
}

// 打印结果
void print_result(int n, int result) {
    if (result) {
        putch(89); // 'Y'
    } else {
        putch(78); // 'N'
    }
    putch(10);
}

int main() {
    // 获取用户输入的整数
    int input = getint();

    // 测试shadow变量
    {
        int input = 42; // 影子变量，不影响外部input
        putint(input);  // 输出影子变量值
        putch(10);
    }

    // 判断输入的整数是否为素数
    int prime = is_prime(input);

    // 打印结果
    print_result(input, prime);

    return 0;
}