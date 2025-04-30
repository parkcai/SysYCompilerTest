/*
 * 测试数组操作、函数递归、逻辑运算和变量shadow
 * 功能：实现斐波那契数列的计算并输出结果
 */

// 计算斐波那契数列
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 输出斐波那契数列
void print_fibonacci(int n) {
    int i = 0;
    while (i < n) {
        putint(fibonacci(i));
        putch(32); // 空格
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    int n = getint();
    
    // 测试shadow变量
    {
        int n = 5; // shadow变量
        putch(83); // 'S'
        putch(104); // 'h'
        putch(97); // 'a'
        putch(100); // 'd'
        putch(111); // 'o'
        putch(119); // 'w'
        putch(58); // ':'
        putch(32); // 空格
        putint(n);
        putch(10); // 换行
        print_fibonacci(n); // 输出shadow变量对应的斐波那契数列
    }
    
    // 输出实际输入值对应的斐波那契数列
    print_fibonacci(n);
    
    return 0;
}