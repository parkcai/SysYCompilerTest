/*
 * 测试用例：实现斐波那契数列计算与输出
 * 功能：用户输入一个整数n，程序计算并输出前n项斐波那契数列
 * 本测试用例旨在验证递归函数、数组操作、循环控制和库函数使用
 */

// 递归计算斐波那契数列
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 使用迭代方式填充斐波那契数组
void fill_fib_array(int arr[], int size) {
    int i = 0;
    while (i < size) {
        if (i == 0 || i == 1) {
            arr[i] = i;
        } else {
            arr[i] = arr[i - 1] + arr[i - 2];
        }
        i = i + 1;
    }
}

// 打印数组内容
void print_array(int arr[], int size) {
    int i = 0;
    while (i < size) {
        putint(arr[i]);
        if (i != size - 1) {
            putch(32); // 空格分隔
        }
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    // 获取用户输入的n值
    int n = getint();
    
    // 测试shadow变量
    {
        int n = 5;
        putch(83); // 'S'
        putch(104); // 'h'
        putch(97); // 'a'
        putch(100); // 'd'
        putch(111); // 'o'
        putch(119); // 'w'
        putch(58); // ':'
        putch(32); // 空格
        putint(n);
        putch(10); // 换行
    }
    
    // 方法1：使用递归计算
    putch(82); // 'R'
    putch(101); // 'e'
    putch(99); // 'c'
    putch(117); // 'u'
    putch(114); // 'r'
    putch(115); // 's'
    putch(105); // 'i'
    putch(118); // 'v'
    putch(101); // 'e'
    putch(58); // ':'
    putch(10); // 换行
    int i = 0;
    while (i < n) {
        putint(fibonacci(i));
        if (i != n - 1) {
            putch(32); // 空格分隔
        }
        i = i + 1;
    }
    putch(10); // 换行
    
    // 方法2：使用数组迭代计算
    int fib_arr[20]; // 假设最大20项
    fill_fib_array(fib_arr, n);
    putch(73); // 'I'
    putch(116); // 't'
    putch(101); // 'e'
    putch(114); // 'r'
    putch(97); // 'a'
    putch(116); // 't'
    putch(105); // 'i'
    putch(118); // 'v'
    putch(101); // 'e'
    putch(58); // ':'
    putch(10); // 换行
    print_array(fib_arr, n);
    
    return 0;
}