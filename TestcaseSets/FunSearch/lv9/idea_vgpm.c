/*
 * 测试数组操作、函数递归、逻辑运算和变量shadow
 * 功能：实现斐波那契数列计算并输出前n项
 */

// 递归计算斐波那契数列
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 初始化斐波那契数组
void init_fib_array(int arr[], int size) {
    int i = 0;
    while (i < size) {
        arr[i] = fibonacci(i);
        i = i + 1;
    }
}

// 打印数组
void print_array(int arr[], int size) {
    int i = 0;
    while (i < size) {
        putint(arr[i]);
        if (i != size - 1) {
            putch(32); // 空格
        }
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    int n = getint(); // 获取要输出的项数
    
    // 测试短路求值
    if (n <= 0 || n > 20) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        putch(10); // 换行
        return 0;
    }
    
    // 定义数组存储斐波那契数列
    int fib[20];
    init_fib_array(fib, n);
    
    // 测试shadow变量
    {
        int n = 5;
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
    }
    
    // 输出斐波那契数列
    print_array(fib, n);
    
    // 测试数组访问
    putint(fib[n-1]); // 输出最后一项
    return 0;
}