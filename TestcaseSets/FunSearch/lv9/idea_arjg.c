/*
 * 测试数组操作、函数递归和逻辑运算的综合程序
 * 功能：实现斐波那契数列计算并验证结果
 */

// 计算斐波那契数列第n项
int fibonacci(int n) {
    if (n == 0) {
        return 0;
    }
    if (n == 1) {
        return 1;
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
        if (i < size - 1) {
            putch(32); // 空格
        }
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    // 测试斐波那契数列
    int fib[10];
    init_fib_array(fib, 10);
    
    putch(70); // 'F'
    putch(105); // 'i'
    putch(98); // 'b'
    putch(111); // 'o'
    putch(110); // 'n'
    putch(97); // 'a'
    putch(99); // 'c'
    putch(99); // 'c'
    putch(105); // 'i'
    putch(58); // ':'
    putch(10); // 换行
    print_array(fib, 10);
    
    // 测试短路求值
    if (fib[5] == 5 || fib[6] != 8 && fib[7] == 13) {
        putch(86); // 'V'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        putch(10); // 换行
    }
    
    // 测试shadow
    {
        int fib[5] = {1, 1, 2, 3, 5};
        putch(83); // 'S'
        putch(104); // 'h'
        putch(97); // 'a'
        putch(100); // 'd'
        putch(111); // 'o'
        putch(119); // 'w'
        putch(58); // ':'
        putch(10); // 换行
        print_array(fib, 5);
    }
    
    return 0;
}