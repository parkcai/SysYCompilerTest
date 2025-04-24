/* 测试数组操作和逻辑运算的综合程序 */
int find_max(int arr[], int len) {
    int max = arr[0];
    int i = 1;
    while (i < len) {
        if (arr[i] > max) {
            max = arr[i];
        }
        i = i + 1;
    }
    return max;
}

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

int main() {
    int arr[5];
    int count = getarray(arr);
    
    // 处理输入不足的情况
    if (count < 5) {
        putch(33); // 输出'!'表示错误
        return -1;
    }
    
    // 测试数组最大值
    int max = find_max(arr, 5);
    putint(max);
    putch(10);
    
    // 测试素数判断
    int j = 0;
    while (j < 5) {
        if (is_prime(arr[j]) && (arr[j] % 2 != 0 || arr[j] == 2)) {
            putint(arr[j]);
            putch(32);
        }
        j = j + 1;
    }
    
    // 测试作用域shadow
    {
        int max = 0;
        putch(10);
        putint(max); // 应输出0而非之前的最大值
    }
    
    return 0;
}