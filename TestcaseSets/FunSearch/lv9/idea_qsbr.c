/*
 * 测试用例：实现斐波那契数列计算器
 * 功能：用户输入一个整数n，程序计算并输出斐波那契数列的第n项
 * 本测试用例旨在验证递归函数、数组操作、短路逻辑和变量shadow
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

// 使用数组缓存优化计算
int fib_with_cache(int n, int cache[]) {
    if (n == 0) {
        return 0;
    }
    if (cache[n] != 0) {
        return cache[n];
    }
    if (n == 1 || n == 2) {
        cache[n] = 1;
        return 1;
    }
    cache[n] = fib_with_cache(n - 1, cache) + fib_with_cache(n - 2, cache);
    return cache[n];
}

int main() {
    int n = getint(); // 获取用户输入
    
    // 测试短路逻辑
    if (n < 0 || n > 20) {
        putint(-1); // 输入不合法
        return 0;
    }
    
    // 定义缓存数组
    int cache[21];
    int i = 0;
    while (i <= 20) {
        cache[i] = 0;
        i = i + 1;
    }
    
    // 计算并输出结果
    putint(fibonacci(n));
    putch(32); // 空格
    putint(fib_with_cache(n, cache));
    putch(10); // 换行
    
    // 测试变量shadow
    {
        int n = 42;
        putint(n); // 应输出42
        putch(10); // 换行
    }
    
    return 0;
}