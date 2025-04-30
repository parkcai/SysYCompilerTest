/*
 * 测试多维数组、函数递归、短路求值和变量shadow
 * 功能：实现矩阵转置和矩阵乘法验证
 */

// 递归初始化矩阵
void init_matrix(int mat[][3], int size, int val) {
    if (size <= 0) return;
    int i = 0;
    while (i < 3) {
        mat[size-1][i] = val + i;
        i = i + 1;
    }
    init_matrix(mat, size-1, val+3);
}

// 矩阵转置
void transpose(int mat[][3], int result[][3], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < 3) {
            result[j][i] = mat[i][j];
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
    int mat1[3][3];
    int mat2[3][3];
    int trans[3][3];
    int result[3][3];
    
    // 初始化矩阵
    init_matrix(mat1, 3, 1);
    init_matrix(mat2, 3, 2);
    
    // 测试shadow
    {
        int mat1 = getint();
        putint(mat1); // 输出输入值
        putch(10);
    }
    
    // 转置矩阵
    transpose(mat1, trans, 3);
    
    // 利用短路特性防止除零错误
    if (mat1[0][0] != 0 && (mat2[0][0] / mat1[0][0] > 0)) {
        // 矩阵乘法
        matrix_multiply(mat1, trans, result, 3);
    } else {
        matrix_multiply(mat1, mat2, result, 3);
    }
    
    // 输出结果
    print_matrix(result, 3);
    
    return 0;
}