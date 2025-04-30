/*
 * 测试递归、数组操作和短路求值
 * 功能：计算斐波那契数列并验证数组操作
 */

// 全局常量数组初始化
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

// 测试数组越界检查
int safe_get(int arr[], int size, int index) {
    if (index < 0 || index >= size) {
        return -1;  // 越界返回-1
    }
    return arr[index];
}

int main() {
    int n = getint();
    int fibs[10];
    
    // 填充斐波那契数组
    fill_fib_array(fibs, 10);
    
    // 输出斐波那契数列
    putarray(10, fibs);
    
    // 测试短路求值
    if (n >= 0 && n < 10) {
        putint(fibs[n]);  // 输出第n项
    } else {
        putch(69); // 'E'
        putch(114); // 'r'
        putch(114); // 'r'
        putch(111); // 'o'
        putch(114); // 'r'
    }
    
    // 测试shadow变量
    {
        int n = 5;
        putint(safe_get(fibs, 10, n));  // 应输出5
    }
    
    return 0;
}