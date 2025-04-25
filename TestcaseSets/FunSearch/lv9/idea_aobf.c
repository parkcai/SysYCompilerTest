/*
 * 测试数组操作、递归函数、逻辑运算和变量shadow的综合应用
 * 功能：计算并输出斐波那契数列的前n项
 */

// 全局常量数组初始化
const int INIT[2] = {0, 1};  // 斐波那契数列前两项

// 递归计算斐波那契数列第n项
int fibonacci(int n) {
    if (n < 2) {
        return INIT[n];  // 访问全局常量数组
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int main() {
    int n = getint(); // 获取要输出的项数
    
    // 测试短路求值
    if (n <= 0 || n > 20) {
        putch(33); // 输出'!'表示错误
        return -1;
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
    
    // 测试数组操作
    putch(10);
    putarray(n, fib); // 输出整个数组
    
    return 0;
}