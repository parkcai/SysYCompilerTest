/*
 * 测试多维数组、函数递归、逻辑运算短路和变量shadow
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
void matrix_multiply(int A[][3], int B[][3], int C[][3], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            int k = 0;
            C[i][j] = 0;
            while (k < size) {
                C[i][j] = C[i][j] + A[i][k] * B[k][j];
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
    int A[3][3];
    int B[3][3];
    int C[3][3];

    // 初始化矩阵A和B
    init_matrix(A, 3);
    init_matrix(B, 3);

    // 测试短路求值
    if (A[0][0] == 1 || B[1][1] != 5) {
        putch(33); // 输出'!'
    }

    // 矩阵乘法
    matrix_multiply(A, B, C, 3);

    // 输出结果
    putch(65); // 'A'
    putch(58); // ':'
    putch(10);
    print_matrix(A, 3);

    putch(66); // 'B'
    putch(58); // ':'
    putch(10);
    print_matrix(B, 3);

    putch(67); // 'C'
    putch(58); // ':'
    putch(10);
    print_matrix(C, 3);

    // 测试shadow
    {
        int A[2][3] = {{1, 2, 3}, {4, 5, 6}};
        putch(83); // 'S'
        putch(58); // ':'
        putch(10);
        print_matrix(A, 2);
    }

    return 0;
}