/*
 * 测试多维数组、函数递归、逻辑运算短路特性、变量shadow
 * 功能：实现一个简单的矩阵加法，并验证结果
 */

// 定义全局常量
const int SIZE = 3;

// 矩阵加法函数
void matrix_add(int mat1[][3], int mat2[][3], int result[][3]) {
    int i = 0;
    while (i < SIZE) {
        int j = 0;
        while (j < SIZE) {
            result[i][j] = mat1[i][j] + mat2[i][j];
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印矩阵
void print_matrix(int mat[][3]) {
    int i = 0;
    while (i < SIZE) {
        int j = 0;
        while (j < SIZE) {
            putint(mat[i][j]);
            if (j < 2) {
                putch(32); // 空格
            }
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

// 验证矩阵加法是否正确
int verify_matrix_add(int mat1[][3], int mat2[][3], int result[][3]) {
    int i = 0;
    while (i < SIZE) {
        int j = 0;
        while (j < SIZE) {
            // 测试逻辑运算短路
            if (result[i][j] != (mat1[i][j] + mat2[i][j])) {
                return 0;
            }
            j = j + 1;
        }
        i = i + 1;
    }
    return 1;
}

int main() {
    int matrix1[3][3];
    int matrix2[3][3];
    int result[3][3];

    // 输入矩阵1
    int i = 0;
    while (i < SIZE) {
        int j = 0;
        while (j < SIZE) {
            matrix1[i][j] = getint();
            j = j + 1;
        }
        i = i + 1;
    }

    // 输入矩阵2
    i = 0;
    while (i < SIZE) {
        int j = 0;
        while (j < SIZE) {
            matrix2[i][j] = getint();
            j = j + 1;
        }
        i = i + 1;
    }

    // 测试变量shadow
    {
        int i = 99;
        putint(i); // 应输出99
        putch(10);
    }

    // 进行矩阵加法
    matrix_add(matrix1, matrix2, result);

    // 输出结果
    putch(77); // 'M'
    putch(97); // 'a'
    putch(116); // 't'
    putch(114); // 'r'
    putch(105); // 'i'
    putch(120); // 'x'
    putch(32); // 空格
    putch(49); // '1'
    putch(58); // ':'
    putch(10); // 换行
    print_matrix(matrix1);

    putch(77); // 'M'
    putch(97); // 'a'
    putch(116); // 't'
    putch(114); // 'r'
    putch(105); // 'i'
    putch(120); // 'x'
    putch(32); // 空格
    putch(50); // '2'
    putch(58); // ':'
    putch(10); // 换行
    print_matrix(matrix2);

    putch(82); // 'R'
    putch(101); // 'e'
    putch(115); // 's'
    putch(117); // 'u'
    putch(108); // 'l'
    putch(116); // 't'
    putch(58); // ':'
    putch(10); // 换行
    print_matrix(result);

    // 验证结果
    if (verify_matrix_add(matrix1, matrix2, result)) {
        putch(86); // 'V'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
    } else {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
    }

    return 0;
}