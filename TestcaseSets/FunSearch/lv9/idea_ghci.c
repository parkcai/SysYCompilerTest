/*
 * 测试用例：实现一个简单的斐波那契数列生成器
 * 功能：计算并输出斐波那契数列的前n项，测试递归调用、数组操作和库函数使用
 */

// 递归实现斐波那契数列
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 打印斐波那契数列
void print_fibonacci(int n) {
    int i = 0;
    while (i < n) {
        putint(fibonacci(i));
        putch(32); // 空格
        i = i + 1;
    }
    putch(10); // 换行
}

// 测试变量shadow
void test_shadow() {
    int n = 5; // 外层变量n
    {
        int n = 10; // 内层变量n
        putint(n); // 应输出10
        putch(10);
    }
    putint(n); // 应输出5
    putch(10);
}

// 主函数
int main() {
    // 获取用户输入的斐波那契数列长度
    int n = getint();
    
    // 测试斐波那契数列生成
    print_fibonacci(n);
    
    // 测试变量shadow
    test_shadow();
    
    return 0;
}