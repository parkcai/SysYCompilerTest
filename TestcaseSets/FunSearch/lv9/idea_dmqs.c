/*
 * 测试数组操作、函数递归、短路求值和变量shadow
 * 功能：计算斐波那契数列并输出前n项
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
        
        // 输出当前项
        putint(fib[i]);
        putch(32); // 空格
        
        // 测试shadow
        {
            int i = 100;
            putint(i); // 应输出100
            putch(10);
        }
        
        i = i + 1;
    }
    
    return 0;
}