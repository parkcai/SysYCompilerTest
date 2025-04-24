/*
 * 测试数组操作、函数递归、逻辑运算短路和变量shadow
 * 功能：计算并输出斐波那契数列前n项
 */

// 递归计算斐波那契数列
int fibonacci(int n) {
    if (n == 0) {
        return 0;
    }
    // 利用短路特性避免无效递归
    if (n == 1 || n == 2) {
        return 1;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 打印斐波那契数列
void print_fibonacci(int n) {
    int i = 0;
    while (i < n) {
        // 测试变量shadow
        {
            int i = 888;
            putint(i);  // 应输出888
            putch(10);  // 换行
        }
        
        putint(fibonacci(i));
        putch(32);  // 空格
        i = i + 1;
    }
    putch(10);  // 换行
}

int main() {
    int n = getint();
    int fib[10];  // 数组存储斐波那契数列
    
    // 测试数组初始化
    {
        int i = 0;
        while (i < 10) {
            fib[i] = fibonacci(i);
            i = i + 1;
        }
        putarray(10, fib);  // 输出前10项斐波那契数列
    }
    
    // 测试输入合法性
    if (n > 0 && n <= 10) {
        print_fibonacci(n);
    } else {
        putint(-1);  // 非法输入
    }
    
    return 0;
}