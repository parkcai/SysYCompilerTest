/*
 * 测试数组初始化、逻辑运算短路求值、递归函数和变量shadow
 * 功能：计算并输出斐波那契数列的前n项
 */

// 全局常量初始化
const int FIB_INIT[2] = {0, 1};  // 斐波那契数列前两项

// 递归计算斐波那契数
int fibonacci(int n) {
    if (n < 2) {
        return FIB_INIT[n];  // 访问全局常量数组
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int main() {
    int n = getint(); // 获取要输出的项数
    
    // 测试短路求值
    if (n <= 0 || n > 15) {
        putint(-1); // 输入不合法
        return 0;
    }
    
    // 定义数组存储斐波那契数列
    int fib_seq[15];
    int i = 0;
    while (i < n) {
        fib_seq[i] = fibonacci(i);
        
        // 输出当前元素
        putint(fib_seq[i]);
        putch(32); // 空格
        
        // 测试shadow
        {
            int i = 100;  // shadow外层i
            putint(i);    // 应输出100
            putch(32);
        }
        
        i = i + 1;
    }
    
    putch(10); // 换行
    
    // 测试数组访问
    putint(fib_seq[n-1]); // 输出最后一项
    return 0;
}