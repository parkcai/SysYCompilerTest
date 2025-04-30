/*
 * 测试一维数组、while循环、逻辑运算短路和变量shadow
 * 功能：计算并输出斐波那契数列的前N项
 */

// 计算斐波那契数列
void fibonacci(int n, int result[]) {
    if (n <= 0) return;
    
    result[0] = 0;
    if (n == 1) return;
    
    result[1] = 1;
    if (n == 2) return;
    
    int i = 2;
    while (i < n) {
        result[i] = result[i - 1] + result[i - 2];
        i = i + 1;
    }
}

// 打印数组
void print_array(int arr[], int n) {
    int i = 0;
    while (i < n) {
        putint(arr[i]);
        putch(32); // 空格
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    int n = getint(); // 获取要计算的斐波那契数列的项数
    
    // 测试短路求值
    if (n <= 0 || n > 20) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        putch(32); // ' '
        putch(105); // 'i'
        putch(110); // 'n'
        putch(112); // 'p'
        putch(117); // 'u'
        putch(116); // 't'
        return 0;
    }
    
    // 测试shadow变量
    {
        int n = 5;
        putint(n); // 应输出5而非输入值
        putch(10);
    }
    
    int fib[20];
    fibonacci(n, fib);
    print_array(fib, n);
    
    return 0;
}