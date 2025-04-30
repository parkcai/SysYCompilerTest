/*
 * 测试SysY语言中的多维数组、函数递归和逻辑运算
 * 功能：实现一个简单的矩阵乘法并验证结果
 */

// 初始化矩阵
void init_matrix(int matrix[][3], int rows) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < 3) {
            matrix[i][j] = i * 3 + j + 1; // 填充1~9
            j = j + 1;
        }
        i = i + 1;
    }
}

// 矩阵乘法
void matrix_multiply(int a[][3], int b[][3], int result[][3], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            int k = 0;
            result[i][j] = 0;
            while (k < size) {
                result[i][j] = result[i][j] + a[i][k] * b[k][j];
                k = k + 1;
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印矩阵
void print_matrix(int matrix[][3], int rows) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < 3) {
            putint(matrix[i][j]);
            if (j < 2) {
                putch(32); // 空格
            }
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    int a[3][3];
    int b[3][3];
    int result[3][3];

    // 初始化矩阵a和b
    init_matrix(a, 3);
    init_matrix(b, 3);

    // 测试短路求值
    if (a[0][0] == 1 && b[1][1] != 5) {
        putch(33); // 输出'!'
    }

    // 矩阵乘法
    matrix_multiply(a, b, result, 3);

    // 输出结果
    putch(65); // 'A'
    putch(58); // ':'
    putch(10);
    print_matrix(a, 3);

    putch(66); // 'B'
    putch(58); // ':'
    putch(10);
    print_matrix(b, 3);

    putch(82); // 'R'
    putch(58); // ':'
    putch(10);
    print_matrix(result, 3);

    // 测试变量shadow
    {
        int a[2][3] = {{1, 2, 3}, {4, 5, 6}};
        putch(83); // 'S'
        putch(58); // ':'
        putch(10);
        print_matrix(a, 2);
    }

    return 0;
}