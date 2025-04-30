/*
 * 测试一维数组、二维数组、逻辑运算短路、递归函数和变量shadow
 * 功能：实现矩阵乘法并验证结果，同时测试逻辑运算短路特性和多维数组
 */

// 矩阵乘法
void matrix_multiply(int a[][3], int b[][3], int c[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            c[i][j] = 0;
            int k = 0;
            while (k < 3) {
                c[i][j] = c[i][j] + a[i][k] * b[k][j];
                k = k + 1;
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 初始化二维数组
void init_2d_array(int mat[][3], int rows, int val) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < 3) {
            mat[i][j] = val + i * 3 + j;
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印二维数组
void print_2d_array(int mat[][3], int rows) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < 3) {
            putint(mat[i][j]);
            putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

// 递归计算斐波那契数列
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int main() {
    int a[3][3] = {0};
    int b[3][3] = {0};
    int c[3][3] = {0};

    // 初始化矩阵a
    init_2d_array(a, 3, 1);
    // 初始化矩阵b
    init_2d_array(b, 3, 4);

    // 计算矩阵乘法
    matrix_multiply(a, b, c);

    // 输出矩阵a
    putch(65); // 'A'
    putch(58); // ':'
    putch(10); // 换行
    print_2d_array(a, 3);

    // 输出矩阵b
    putch(66); // 'B'
    putch(58); // ':'
    putch(10); // 换行
    print_2d_array(b, 3);

    // 输出矩阵c
    putch(67); // 'C'
    putch(58); // ':'
    putch(10); // 换行
    print_2d_array(c, 3);

    // 测试逻辑运算短路特性
    int x = getint();
    int y = getint();
    if (x > 0 && y > 0 || x == y) {
        putint(1);
    } else {
        putint(0);
    }

    // 测试变量shadow
    {
        int x = 5;
        putint(x); // 应输出5
        putch(10);
    }

    // 测试斐波那契数列
    int n = getint();
    putint(fibonacci(n));

    return 0;
}