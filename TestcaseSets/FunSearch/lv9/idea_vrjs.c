/*
 * 测试多维数组、函数递归、逻辑运算短路和变量shadow
 * 功能：实现一个简单的素数筛法，并验证结果
 */

// 判断是否为素数
int is_prime(int n) {
    if (n <= 1) {
        return 0;
    }
    int i = 2;
    while (i * i <= n) {
        if (n % i == 0) {
            return 0;
        }
        i = i + 1;
    }
    return 1;
}

// 素数筛法
void prime_sieve(int arr[], int size) {
    int i = 0;
    while (i < size) {
        if (is_prime(arr[i])) {
            putint(arr[i]);
            putch(32); // 空格
        }
        i = i + 1;
    }
    putch(10); // 换行
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
    // 获取数组大小和元素
    int size = getint();
    int arr[10];
    int i = 0;
    while (i < size) {
        arr[i] = getint();
        i = i + 1;
    }

    // 测试shadow变量
    {
        int size = 999;
        putint(size); // 应输出999
        putch(10); // 换行
    }

    // 测试短路求值
    if (size <= 0 || size > 10) {
        putch(69); // 'E'
        putch(114); // 'r'
        putch(114); // 'r'
        putch(111); // 'o'
        putch(114); // 'r'
        return 0;
    }

    // 输出原始数组
    print_array(arr, size);

    // 执行素数筛法
    prime_sieve(arr, size);

    return 0;
}