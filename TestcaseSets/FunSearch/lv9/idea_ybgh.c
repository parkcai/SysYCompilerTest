/*
 * 测试数组初始化、逻辑运算短路、递归函数和变量shadow
 * 功能：计算并输出斐波那契数列前n项
 */

// 递归计算斐波那契数列
int fib(int n, int memo[]) {
    if (n <= 1) {
        return n;
    }
    // 利用短路特性防止数组越界
    if (memo[n] == 0 && n > 1) {
        memo[n] = fib(n-1, memo) + fib(n-2, memo);
    }
    return memo[n];
}

// 初始化数组并计算斐波那契数列
void init_fib(int arr[], int size) {
    int i = 0;
    while (i < size) {
        arr[i] = fib(i, arr);
        i = i + 1;
    }
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
    if (n <= 0 || n > 20) {
        putint(-1); // 非法输入
        return 0;
    }
    
    int fib_arr[20] = {0}; // 初始化数组
    init_fib(fib_arr, n);
    
    // 测试变量shadow
    {
        int n = 999;
        putint(n); // 应输出999
        putch(10); // 换行
    }
    
    print_array(fib_arr, n);
    return 0;
}