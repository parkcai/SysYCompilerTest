/*
 * 测试递归函数、数组操作、短路求值和变量shadow
 * 功能：计算斐波那契数列并验证结果
 */

// 递归计算斐波那契数列
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 验证斐波那契数列计算结果
int verify_fibonacci(int arr[], int size) {
    int i = 2;
    while (i < size) {
        // 测试短路求值
        if (arr[i] != (arr[i-1] + arr[i-2]) && i < 100) {
            return 0;
        }
        i = i + 1;
    }
    return 1;
}

int main() {
    int n = getint(); // 获取要计算的斐波那契数列长度
    
    // 测试输入合法性
    if (n <= 0 || n > 20) {
        putint(-1);
        return 0;
    }
    
    // 定义数组存储斐波那契数列
    int fib[20];
    int i = 0;
    while (i < n) {
        fib[i] = fibonacci(i);
        
        // 输出当前斐波那契数
        putint(fib[i]);
        putch(32); // 空格
        
        // 测试变量shadow
        {
            int i = 999;
            if (i > 500 || n < 0) { // 测试短路求值
                putint(i); // 应输出999
                putch(32);
            }
        }
        
        i = i + 1;
    }
    putch(10); // 换行
    
    // 验证计算结果
    if (verify_fibonacci(fib, n)) {
        putch(86); // 'V'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
    } else {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
    }
    
    return 0;
}