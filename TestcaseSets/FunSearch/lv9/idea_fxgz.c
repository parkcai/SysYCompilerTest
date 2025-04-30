/*
 * 测试用例：实现一个简单的斐波那契数列生成器
 * 功能：用户输入一个整数n，程序输出斐波那契数列的前n项。
 * 本测试用例旨在验证SysY语言中的函数定义与调用、逻辑运算和数组操作。
 */

// 计算斐波那契数列的第n项
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    int a = 0, b = 1, c = 0;
    int i = 2;
    while (i <= n) {
        c = a + b;
        a = b;
        b = c;
        i = i + 1;
    }
    return c;
}

// 打印斐波那契数列的前n项
void print_fibonacci(int n) {
    int i = 0;
    while (i < n) {
        putint(fibonacci(i));
        if (i < n - 1) {
            putch(32); // 空格
        }
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    // 获取用户输入的整数
    int n = getint();

    // 测试shadow变量
    {
        int n = 42; // 影子变量，不影响外部n
        putint(n);  // 输出影子变量值
        putch(10);
    }

    // 打印斐波那契数列的前n项
    print_fibonacci(n);

    // 测试短路求值
    if (getint() != 0 && getint() != 0) {
        putint(1);
    } else {
        putint(0);
    }

    return 0;
}