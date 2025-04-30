/*
 * 测试数组操作、函数递归、逻辑运算短路和变量shadow
 * 功能：实现并测试斐波那契数列的计算
 */

// 计算斐波那契数列
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 打印斐波那契数列
void print_fibonacci(int n) {
    int i = 0;
    while (i < n) {
        putint(fibonacci(i));
        putch(32); // 空格
        i = i + 1;
    }
    putch(10); // 换行
}

// 测试数组初始化和访问
void test_array() {
    const int arr[3][2] = {{1, 2}, {3, 4}, {5, 6}};
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 2) {
            putint(arr[i][j]);
            putch(32); // 空格
            j = j + 1;
        }
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    int n = getint(); // 获取斐波那契数列的长度

    // 测试短路求值
    if (n <= 0 || n > 10) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        return 0;
    }

    // 测试shadow变量
    {
        int n = 999;
        putint(n); // 应输出999
        putch(10); // 换行
    }

    test_array();
    print_fibonacci(n);

    return 0;
}