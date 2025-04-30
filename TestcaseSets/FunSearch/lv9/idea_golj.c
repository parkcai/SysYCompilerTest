/*
 * 测试一维数组初始化、逻辑运算短路求值、变量shadow和递归函数
 * 功能：计算并输出斐波那契数列的前n项
 */

// 计算斐波那契数列第n项的递归实现
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int main() {
    int n = getint(); // 获取要输出的项数
    int i = 0;
    
    // 测试短路求值
    if (n <= 0 || n > 20) {
        putint(-1); // 输入不合法
        return 0;
    }
    
    // 定义一维数组存储斐波那契数列
    int fib[20];
    
    while (i < n) {
        fib[i] = fibonacci(i);
        
        // 输出当前元素
        putint(fib[i]);
        putch(32); // 空格
        
        i = i + 1;
    }
    putch(10); // 换行
    
    // 测试shadow
    {
        int i = 99;
        putint(i); // 应输出99
        putch(10);
    }
    
    return 0;
}