/*
 * 测试一维数组、二维数组、逻辑运算短路、递归函数和变量shadow
 * 功能：实现矩阵转置并验证结果，同时测试逻辑运算短路特性和多维数组
 */

// 矩阵转置
void matrix_transpose(int a[][3], int b[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            b[j][i] = a[i][j];
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

// 递归计算阶乘
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    int a[3][3] = {0};
    int b[3][3] = {0};

    // 初始化矩阵a
    init_2d_array(a, 3, 1);

    // 输出矩阵a
    putch(65); // 'A'
    putch(58); // ':'
    putch(10); // 换行
    print_2d_array(a, 3);

    // 计算矩阵转置
    matrix_transpose(a, b);

    // 输出矩阵b
    putch(66); // 'B'
    putch(58); // ':'
    putch(10); // 换行
    print_2d_array(b, 3);

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

    // 测试阶乘
    int n = getint();
    putint(factorial(n));

    return 0;
}