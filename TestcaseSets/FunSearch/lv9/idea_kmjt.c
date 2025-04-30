/*
 * 测试数组初始化、逻辑运算短路、递归函数和变量shadow
 * 功能：计算并输出给定整数的阶乘，同时测试数组操作和逻辑运算
 */

// 递归计算阶乘
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

// 初始化数组并计算阶乘
void init_factorial(int arr[], int size) {
    int i = 0;
    while (i < size) {
        arr[i] = factorial(i);
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
    int n = getint();
    if (n <= 0 || n > 10) {
        putint(-1); // 非法输入
        return 0;
    }
    
    int fact_arr[10] = {0}; // 初始化数组
    init_factorial(fact_arr, n);
    
    // 测试变量shadow
    {
        int n = 999;
        putint(n); // 应输出999
        putch(10); // 换行
    }
    
    print_array(fact_arr, n);
    return 0;
}