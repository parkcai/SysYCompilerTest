/*
 * 测试数组操作、函数递归和逻辑运算短路
 * 功能：计算并输出斐波那契数列的前n项
 */

// 递归计算斐波那契数列
int fibonacci(int n, int memo[]) {
    if (n == 0 || n == 1) {
        memo[n] = n;
        return n;
    }
    
    // 利用memo数组避免重复计算
    if (memo[n] == 0) {
        memo[n] = fibonacci(n - 1, memo) + fibonacci(n - 2, memo);
    }
    return memo[n];
}

// 打印数组
void print_array(int arr[], int size) {
    int i = 0;
    while (i < size) {
        putint(arr[i]);
        if (i != size - 1) {
            putch(32); // 空格分隔
        }
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    // 测试shadow变量
    int n = getint();
    {
        int n = 10; // shadow外部n
        putint(n);  // 应输出10
        putch(10);
    }
    
    // 测试短路特性
    if (n > 0 && n <= 20) {
        int fib[20]; // 假设最大20项
        int i = 0;
        while (i <= n) {
            fib[i] = 0; // 初始化
            i = i + 1;
        }
        
        // 计算斐波那契数列
        fibonacci(n, fib);
        
        // 打印结果
        print_array(fib, n + 1);
    } else {
        putch(33); // 输出'!'表示无效输入
    }
    
    return 0;
}