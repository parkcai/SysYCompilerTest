/*
 * 测试用例：实现斐波那契数列计算并验证
 * 功能：通过递归和迭代两种方法计算斐波那契数列，并比较结果是否一致
 */

// 递归方法计算斐波那契数列
int fib_recursive(int n) {
    if (n <= 1) {
        return n;
    } else {
        return fib_recursive(n - 1) + fib_recursive(n - 2);
    }
}

// 迭代方法计算斐波那契数列
int fib_iterative(int n) {
    if (n <= 1) {
        return n;
    }
    int a = 0, b = 1, c = 0;
    int i = 2;
    while (i <= n) {
        c = a + b;
        a = b;
        b = c;
        i = i + 1;
    }
    return c;
}

// 比较两个整数是否相等
void compare_fib(int n) {
    int fib_rec = fib_recursive(n);
    int fib_it = fib_iterative(n);
    
    // 使用短路逻辑运算符判断结果是否一致
    if (fib_rec == fib_it && fib_rec != -1) {
        putch(84); // 'T'
        putch(114); // 'r'
        putch(117); // 'u'
        putch(101); // 'e'
    } else {
        putch(70); // 'F'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(115); // 's'
        putch(101); // 'e'
    }
    putch(10); // 换行
}

// 主函数
int main() {
    int n = getint();
    
    // 测试变量 shadow
    {
        int n = 5;
        putint(n); // 应输出5而非输入值
        putch(10);
    }
    
    // 计算并比较斐波那契数列
    compare_fib(n);
    
    // 使用库函数输出结果
    putint(fib_recursive(n));
    putch(32);
    putint(fib_iterative(n));
    putch(10);
    
    return 0;
}