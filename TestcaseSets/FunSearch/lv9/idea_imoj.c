/*
 * 测试多维数组、函数递归、短路求值和变量shadow
 * 功能：实现矩阵转置并验证结果
 */

// 矩阵转置函数
void matrix_transpose(int a[][3], int result[][3], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            result[j][i] = a[i][j];
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印矩阵
void print_matrix(int mat[][3], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            putint(mat[i][j]);
            if (j < size - 1) {
                putch(32); // 空格
            }
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

// 递归计算阶乘
int factorial(int n) {
    if (n == 0 || n == 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    // 定义一个3x3矩阵
    int a[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    int result[3][3];

    // 测试shadow变量
    {
        int a = 100;
        putint(a); // 应输出100
        putch(10);
    }

    // 矩阵转置
    matrix_transpose(a, result, 3);

    // 测试短路求值
    if (result[0][0] > 0 && result[2][2] < 10) {
        print_matrix(result, 3);
    } else {
        putch(33); // 输出'!'表示异常
    }

    // 测试阶乘
    int n = getint();
    putint(factorial(n));

    return 0;
}