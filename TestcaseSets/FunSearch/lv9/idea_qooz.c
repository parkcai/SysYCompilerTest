/*
 * 测试用例：计算阶乘并验证结果
 * 功能：实现递归和非递归两种方式计算阶乘，并输出结果，同时测试作用域shadow行为
 */

// 递归计算阶乘
int factorial_recursive(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial_recursive(n - 1);
}

// 非递归计算阶乘
int factorial_iterative(int n) {
    int result = 1;
    while (n > 1) {
        result = result * n;
        n = n - 1;
    }
    return result;
}

// 打印数组元素
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
    int n = getint(); // 获取输入的整数

    // 测试短路求值
    if (n < 0 || n > 12) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        return 0;
    }

    // 计算阶乘
    int fact_rec = factorial_recursive(n);
    int fact_iter = factorial_iterative(n);

    // 输出结果
    putint(fact_rec);
    putch(32); // 空格
    putint(fact_iter);
    putch(10); // 换行

    // 测试shadow变量
    {
        int n = 99;
        putint(n); // 应输出99
        putch(10); // 换行
    }

    // 测试数组操作
    int arr[10];
    int i = 0;
    while (i < 10) {
        arr[i] = i * i;
        i = i + 1;
    }
    print_array(arr, 10);

    return 0;
}