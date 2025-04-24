/*
 * 测试多维数组、递归函数、短路求值与变量shadow
 * 功能：统计输入数组中的素数，并验证数组操作
 */

const int primes[4] = {2, 3, 5, 7}; // 编译期常量数组

// 判断是否为素数（递归实现）
int is_prime(int n, int divisor) {
    if (n <= 1) return 0;
    if (divisor * divisor > n) return 1;
    if (n % divisor == 0) return 0;
    return is_prime(n, divisor + 1);
}

// 统计素数个数（带数组参数）
int count_primes(int arr[], int len) {
    int cnt = 0;
    int i = 0;
    while (i < len) {
        // 短路测试：当arr[i]<2时跳过判断
        if (arr[i] >= 2 && is_prime(arr[i], 2)) {
            cnt = cnt + 1;
        }
        i = i + 1;
        
        /* 测试continue功能 */
        if (i % 3 == 0) continue;
    }
    return cnt;
}

int main() {
    int data[5] = {0}; // 部分初始化
    int size = getarray(data); // 获取输入数组
    
    // 多维数组与shadow测试
    {
        int data[2][2] = {{11, 13}, {17}}; // shadow一维数组
        putarray(4, data[0]); // 输出11 13 17 0
    }
    
    // 全局常量访问测试
    putint(primes[2]); // 应输出5
    putch(10); // 换行
    
    // 递归函数测试
    int result = count_primes(data, size);
    putint(result);
    
    return 0;
}