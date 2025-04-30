/*
 * 测试用例：斐波那契数列计算
 * 功能：测试递归函数、数组操作、短路逻辑和变量shadow
 */

// 递归计算斐波那契数列
int fibonacci(int n) {
    if (n == 0) {
        return 0;
    }
    if (n == 1 || n == 2) {
        return 1;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 使用数组存储斐波那契数列
void fill_fibonacci(int arr[], int n) {
    int i = 0;
    while (i < n) {
        arr[i] = fibonacci(i);
        i = i + 1;
    }
}

// 测试短路逻辑运算
void test_short_circuit(int x) {
    if (x != 0 && (10 / x > 1)) {
        putint(1);
    } else {
        putint(0);
    }
    putch(10);
}

int main() {
    int n = getint();
    int arr[20];
    
    // 测试变量shadow
    {
        int n = 10;
        putint(n);
        putch(10);
    }
    
    // 填充斐波那契数列
    fill_fibonacci(arr, n);
    
    // 输出数组
    putarray(n, arr);
    
    // 测试短路逻辑
    test_short_circuit(5);
    test_short_circuit(0);
    
    return 0;
}