/*
 * 测试一维数组初始化、函数调用、逻辑运算和库函数综合应用
 * 功能：计算并输出斐波那契数列的前n项，并检查输入的有效性
 */

// 计算斐波那契数列
int fibonacci(int n) {
    if (n <= 1) return n;
    int a = 0, b = 1, c;
    while (n > 1) {
        c = a + b;
        a = b;
        b = c;
        n = n - 1;
    }
    return b;
}

// 初始化并打印斐波那契数列
void print_fibonacci(int arr[], int n) {
    int i = 0;
    while (i < n) {
        arr[i] = fibonacci(i);
        putint(arr[i]);
        putch(32); // 输出空格
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    int n = getint(); // 获取斐波那契数列的项数
    
    // 测试输入合法性
    if (n <= 0 || n > 20) {
        putch(33); // 输出'!'表示错误
        return -1;
    }
    
    int fib[20]; // 定义一维数组
    print_fibonacci(fib, n);
    
    // 测试shadow变量
    {
        int n = 10;
        putint(n); // 应输出10而非输入值
        putch(10);
    }
    
    return 0;
}