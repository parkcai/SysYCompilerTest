/*
 * 测试数组操作、函数递归、短路逻辑和变量shadow
 * 功能：实现斐波那契数列计算并输出结果
 */

// 计算斐波那契数列第n项
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 打印斐波那契数列前n项
void print_fibonacci(int n) {
    int i = 0;
    while (i < n) {
        putint(fibonacci(i));
        if (i != n - 1) {
            putch(32); // 空格
        }
        i = i + 1;
    }
    putch(10); // 换行
}

// 测试数组初始化和shadow
void test_array() {
    int arr[5] = {0, 1, 2, 3, 4};
    putarray(5, arr); // 输出0 1 2 3 4
    
    {
        int arr[3] = {5, 6, 7};
        putarray(3, arr); // 输出5 6 7
    }
}

int main() {
    int n = getint(); // 获取斐波那契数列项数
    
    // 测试短路逻辑和输入合法性
    if (n <= 0 || (n > 20 && fibonacci(20) % 2 == 0)) {
        putch(69); // 'E'
        putch(114); // 'r'
        putch(114); // 'r'
        putch(111); // 'o'
        putch(114); // 'r'
        return 0;
    }
    
    // 测试数组操作
    test_array();
    
    // 输出斐波那契数列
    print_fibonacci(n);
    
    return 0;
}