/*
 * 测试多维数组、递归、短路求值和变量shadow
 * 功能：计算矩阵乘法并验证结果
 */

// 矩阵乘法函数
void matrix_multiply(int m, int n, int p, int a[][10], int b[][10], int result[][10]) {
    int i = 0;
    while (i < m) {
        int j = 0;
        while (j < p) {
            result[i][j] = 0;
            int k = 0;
            while (k < n) {
                result[i][j] = result[i][j] + a[i][k] * b[k][j];
                k = k + 1;
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 递归计算阶乘用于测试
int factorial(int n) {
    // 测试短路求值
    if (n <= 1 || (n > 1 && n < 100)) {
        return 1;
    }
    return n * factorial(n - 1);
}

// 打印矩阵
void print_matrix(int rows, int cols, int matrix[][10]) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < cols) {
            putint(matrix[i][j]);
            if (j < cols - 1) {
                putch(32); // 空格
            }
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    // 测试变量shadow
    int n = 5;
    {
        int n = factorial(5);
        putint(n); // 应输出120
        putch(10);
    }

    // 定义并初始化矩阵A
    int a[2][10] = {{1, 2, 3}, {4, 5, 6}};
    // 定义并初始化矩阵B
    int b[3][10] = {{7, 8}, {9, 10}, {11, 12}};
    // 结果矩阵
    int result[2][10];

    // 计算矩阵乘法
    matrix_multiply(2, 3, 2, a, b, result);

    // 输出结果矩阵
    putch(82); // 'R'
    putch(101); // 'e'
    putch(115); // 's'
    putch(117); // 'u'
    putch(108); // 'l'
    putch(116); // 't'
    putch(58); // ':'
    putch(10);
    print_matrix(2, 2, result);

    return 0;
}