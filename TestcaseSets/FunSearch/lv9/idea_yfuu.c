/*
 * 测试数组初始化、逻辑运算短路、递归函数和变量shadow
 * 功能：计算并输出斐波那契数列的前n项
 */

// 递归计算斐波那契数列
int fibonacci(int n, int fib[]) {
    if (n == 0) {
        fib[0] = 0;
        return 0;
    }
    if (n == 1) {
        fib[1] = 1;
        return 1;
    }
    fib[n] = fibonacci(n-1, fib) + fibonacci(n-2, fib);
    return fib[n];
}

// 打印数组
void print_array(int arr[], int size) {
    int i = 0;
    while (i < size) {
        putint(arr[i]);
        putch(32); // 空格
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    int n = getint();
    int fib[20]; // 存储斐波那契数列
    
    // 测试输入合法性
    if (n <= 0 || n > 20) {
        putint(-1); // 非法输入
        return 0;
    }
    
    // 测试变量shadow
    {
        int n = 5;
        putint(n); // 应输出5
        putch(10);
    }
    
    // 计算斐波那契数列
    fibonacci(n-1, fib);
    
    // 输出结果
    print_array(fib, n);
    
    return 0;
}