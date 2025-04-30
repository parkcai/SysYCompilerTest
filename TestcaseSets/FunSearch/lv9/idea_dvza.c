/*
 * 测试多维数组操作、函数递归、短路逻辑运算和变量shadow
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
void matrix_multiply(int a[][3], int b[][3], int result[][3], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            result[i][j] = 0;
            int k = 0;
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

int main() {
    int a[3][3];
    int b[3][3];
    int result[3][3];

    // 初始化矩阵a和b
    init_matrix(a, 3);
    init_matrix(b, 3);

    // 打印原始矩阵a和b
    putch(79); // 'O'
    putch(114); // 'r'
    putch(105); // 'i'
    putch(103); // 'g'
    putch(105); // 'i'
    putch(110); // 'n'
    putch(97); // 'a'
    putch(108); // 'l'
    putch(32); // 空格
    putch(77); // 'M'
    putch(97); // 'a'
    putch(116); // 't'
    putch(114); // 'r'
    putch(105); // 'i'
    putch(120); // 'x'
    putch(32); // 空格
    putch(65); // 'A'
    putch(58); // ':'
    putch(10); // 换行
    print_matrix(a, 3);

    putch(79); // 'O'
    putch(114); // 'r'
    putch(105); // 'i'
    putch(103); // 'g'
    putch(105); // 'i'
    putch(110); // 'n'
    putch(97); // 'a'
    putch(108); // 'l'
    putch(32); // 空格
    putch(77); // 'M'
    putch(97); // 'a'
    putch(116); // 't'
    putch(114); // 'r'
    putch(105); // 'i'
    putch(120); // 'x'
    putch(32); // 空格
    putch(66); // 'B'
    putch(58); // ':'
    putch(10); // 换行
    print_matrix(b, 3);

    // 矩阵乘法
    matrix_multiply(a, b, result, 3);

    // 打印结果矩阵
    putch(82); // 'R'
    putch(101); // 'e'
    putch(115); // 's'
    putch(117); // 'u'
    putch(108); // 'l'
    putch(116); // 't'
    putch(32); // 空格
    putch(77); // 'M'
    putch(97); // 'a'
    putch(116); // 't'
    putch(114); // 'r'
    putch(105); // 'i'
    putch(120); // 'x'
    putch(58); // ':'
    putch(10); // 换行
    print_matrix(result, 3);

    // 测试短路逻辑
    if (result[0][0] == 30 && result[1][1] == 78 || result[2][2] != 126) {
        putch(33); // '!'
        putch(10);
    }

    // 测试shadow变量
    {
        int a = 999;
        putint(a); // 应输出999
        putch(10); // 换行
    }

    return 0;
}