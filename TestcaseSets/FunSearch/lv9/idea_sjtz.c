/*
 * 测试数组操作、递归函数、短路求值和变量shadow
 * 功能：计算并输出斐波那契数列的前n项
 */

// 递归计算斐波那契数列
int fibonacci(int n) {
    if (n == 0 || n == 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int main() {
    int n = getint(); // 获取要输出的项数
    
    // 测试短路求值
    if (n <= 0 || n > 20) {
        putint(-1); // 输入不合法
        return 0;
    }
    
    // 定义数组存储斐波那契数列
    int fib[20];
    int i = 0;
    while (i < n) {
        fib[i] = fibonacci(i);
        
        // 输出当前元素
        putint(fib[i]);
        putch(32); // 空格
        
        // 测试shadow
        {
            int i = 50;
            if (i > 0 && n < 100) { // 测试短路求值
                putint(i); // 应输出50
                putch(32);
            }
        }
        
        i = i + 1;
    }
    
    // 测试数组访问
    putch(10); // 换行
    putint(fib[n-1]); // 输出最后一项
    return 0;
}