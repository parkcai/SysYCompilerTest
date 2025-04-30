/*
 * 测试多维数组操作、函数递归、逻辑运算短路和变量shadow
 * 功能：实现一个简单的矩阵乘法并验证结果
 */

// 初始化矩阵
void init_matrix(int mat[][3], int rows) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < 3) {
            mat[i][j] = i * 3 + j + 1;
            j = j + 1;
        }
        i = i + 1;
    }
}

// 矩阵乘法
void matrix_multiply(int a[][3], int b[][3], int result[][3], int rows) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < 3) {
            result[i][j] = 0;
            int k = 0;
            while (k < 3) {
                result[i][j] = result[i][j] + a[i][k] * b[k][j];
                k = k + 1;
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印矩阵
void print_matrix(int mat[][3], int rows) {
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

int main() {
    // 测试二维数组
    int matrix_a[3][3];
    int matrix_b[3][3];
    int result[3][3];

    // 初始化并打印原始矩阵
    init_matrix(matrix_a, 3);
    init_matrix(matrix_b, 3);
    
    putch(76); // 'L'
    putch(101); // 'e'
    putch(102); // 'f'
    putch(116); // 't'
    putch(32); // ' '
    putch(109); // 'm'
    putch(97); // 'a'
    putch(116); // 't'
    putch(114); // 'r'
    putch(105); // 'i'
    putch(120); // 'x'
    putch(58); // ':'
    putch(10); // 换行
    print_matrix(matrix_a, 3);

    putch(82); // 'R'
    putch(105); // 'i'
    putch(103); // 'g'
    putch(104); // 't'
    putch(32); // ' '
    putch(109); // 'm'
    putch(97); // 'a'
    putch(116); // 't'
    putch(114); // 'r'
    putch(105); // 'i'
    putch(120); // 'x'
    putch(58); // ':'
    putch(10); // 换行
    print_matrix(matrix_b, 3);

    // 矩阵乘法
    matrix_multiply(matrix_a, matrix_b, result, 3);

    // 打印结果
    putch(82); // 'R'
    putch(101); // 'e'
    putch(115); // 's'
    putch(117); // 'u'
    putch(108); // 'l'
    putch(116); // 't'
    putch(58); // ':'
    putch(10); // 换行
    print_matrix(result, 3);

    // 测试短路求值
    if (result[0][0] == 14 && result[1][1] == 40 || result[2][2] == 66) {
        putch(86); // 'V'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        putch(10); // 换行
    }

    // 测试变量shadow
    {
        int result = 123;
        putint(result); // 应输出123
        putch(10);
    }

    return 0;
}