/*
 * 测试多维数组、函数递归、逻辑运算和库函数综合应用
 * 功能：计算并输出斐波那契数列的前n项
 */

// 递归计算斐波那契数
int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 打印斐波那契数列
void print_fibonacci(int n) {
    int i = 0;
    while (i < n) {
        putint(fibonacci(i));
        putch(32); // 输出空格
        i = i + 1;
    }
    putch(10); // 换行
}

// 测试多维数组初始化和访问
void test_array() {
    int arr[5][5];
    int i = 0;
    while (i < 5) {
        int j = 0;
        while (j < 5) {
            arr[i][j] = i * 5 + j;
            j = j + 1;
        }
        i = i + 1;
    }
    
    // 打印多维数组
    i = 0;
    while (i < 5) {
        int j = 0;
        while (j < 5) {
            putint(arr[i][j]);
            putch(32); // 输出空格
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    int n = getint(); // 获取斐波那契数列的项数
    
    // 测试输入合法性
    if (n <= 0 || n > 20) {
        putch(33); // 输出'!'表示错误
        return -1;
    }
    
    // 输出斐波那契数列
    print_fibonacci(n);
    
    // 测试shadow变量
    {
        int n = 10;
        putint(n); // 应输出10而非输入值
        putch(10);
    }
    
    // 测试多维数组
    test_array();
    
    return 0;
}