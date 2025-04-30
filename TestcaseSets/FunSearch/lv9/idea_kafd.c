/*
 * 测试一维数组、逻辑运算短路、递归函数和变量shadow
 * 功能：计算并输出前n个整数的平方和
 */

// 递归计算平方和
int sum_of_squares(int n) {
    if (n <= 0) {
        return 0;
    }
    return n * n + sum_of_squares(n - 1);
}

// 初始化数组并计算平方和
void init_squares(int arr[], int size) {
    int i = 0;
    while (i < size) {
        arr[i] = (i + 1) * (i + 1);
        i = i + 1;
    }
}

// 打印数组
void print_array(int arr[], int size) {
    int i = 0;
    while (i < size) {
        putint(arr[i]);
        putch(32); // 空格
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    int n = getint();
    if (n <= 0 || n > 10) {
        putint(-1); // 非法输入
        return 0;
    }

    int squares[10] = {0}; // 初始化数组
    init_squares(squares, n);

    // 测试变量shadow
    {
        int n = 999;
        putint(n); // 应输出999
        putch(10); // 换行
    }

    // 输出平方数组
    print_array(squares, n);

    // 计算并输出平方和
    int result = sum_of_squares(n);
    putint(result);

    return 0;
}