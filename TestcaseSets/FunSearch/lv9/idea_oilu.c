/*
 * 测试递归、数组操作、短路求值和变量shadow
 * 功能：计算斐波那契数列并验证数组操作
 */

// 全局常量数组
const int FIB_SEED[2] = {0, 1};

// 递归计算斐波那契数
int fibonacci(int n) {
    if (n < 2) {
        return FIB_SEED[n];  // 访问全局常量数组
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 填充斐波那契数组
void fill_fib_array(int arr[], int size) {
    int i = 0;
    while (i < size) {
        arr[i] = fibonacci(i);
        i = i + 1;
    }
}

// 测试数组越界访问
int safe_array_access(int arr[], int index, int size) {
    if (index >= 0 && index < size) {  // 测试短路求值
        return arr[index];
    } else {
        return -1;
    }
}

int main() {
    int n = getint();  // 获取斐波那契数列长度
    
    // 验证输入有效性
    if (n <= 0 || n > 20) {
        putch(69); // 'E'
        putch(114); // 'r'
        putch(114); // 'r'
        putch(111); // 'o'
        putch(114); // 'r'
        return 0;
    }
    
    int fib_array[20];
    fill_fib_array(fib_array, n);
    
    // 输出斐波那契数列
    putarray(n, fib_array);
    
    // 测试shadow和数组操作
    {
        int n = 5;
        int shadow_array[5] = {10, 20, 30, 40, 50};
        putint(safe_array_access(shadow_array, n - 1, n));  // 应输出50
        putch(10);
    }
    
    // 测试越界访问
    putint(safe_array_access(fib_array, n, n));  // 应输出-1
    return 0;
}