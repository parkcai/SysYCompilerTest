/*
 * 测试用例：实现一个简单的斐波那契数列计算器
 * 功能：用户输入一个整数n，程序计算并输出斐波那契数列的第n项
 * 本测试用例旨在验证SysY语言中的递归函数、逻辑运算、变量shadow和数组操作
 */

// 计算斐波那契数列的递归实现
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 验证输入合法性
int validate_input(int n) {
    if (n < 0 || n > 20) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        putch(33); // '!'
        putch(10); // 换行
        return 0;
    }
    return 1;
}

int main() {
    int n = getint(); // 获取用户输入
    
    // 测试短路逻辑
    if (validate_input(n) && n > 0) {
        // 计算并输出结果
        int result = fibonacci(n);
        putint(result);
        putch(10);
        
        // 测试shadow变量
        {
            int n = 100;
            putint(n); // 应输出100
            putch(10);
        }
    }
    
    return 0;
}