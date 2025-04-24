/*
 * 测试数组操作、函数递归和短路求值
 * 功能：实现斐波那契数列计算和数组反转
 */

// 递归计算斐波那契数列
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 反转数组
void reverse_array(int arr[], int size) {
    int i = 0;
    while (i < size / 2) {
        int temp = arr[i];
        arr[i] = arr[size - i - 1];
        arr[size - i - 1] = temp;
        i = i + 1;
    }
}

// 打印数组
void print_array(int arr[], int size) {
    int i = 0;
    while (i < size) {
        putint(arr[i]);
        if (i < size - 1) {
            putch(32); // 空格
        }
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    // 测试斐波那契数列
    int n = getint();
    
    // 测试短路求值
    if (n <= 0 || n > 10) {
        putint(-1);
        return 0;
    }
    
    int fib[10];
    int i = 0;
    while (i < n) {
        fib[i] = fibonacci(i);
        i = i + 1;
    }
    
    // 测试shadow
    {
        int i = 5;
        putint(i); // 应输出5
        putch(10);
    }
    
    putch(70); // 'F'
    putch(105); // 'i'
    putch(98); // 'b'
    putch(58); // ':'
    putch(32); // ' '
    print_array(fib, n);
    
    // 测试数组反转
    reverse_array(fib, n);
    putch(82); // 'R'
    putch(101); // 'e'
    putch(118); // 'v'
    putch(58); // ':'
    putch(32); // ' '
    print_array(fib, n);
    
    return 0;
}