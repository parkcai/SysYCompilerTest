/*
 * 测试多维数组、递归函数和复杂逻辑表达式
 * 功能：实现矩阵乘法和转置操作
 */

// 矩阵转置函数
void transpose(int mat[][3], int result[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            result[j][i] = mat[i][j];
            j = j + 1;
        }
        i = i + 1;
    }
}

// 矩阵乘法函数
void matrix_mult(int a[][3], int b[][3], int result[][3]) {
    int i = 0;
    while (i < 3) {
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
void print_matrix(int mat[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
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

int main() {
    // 初始化两个3x3矩阵
    int mat1[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    int mat2[3][3] = {{9, 8, 7}, {6, 5, 4}, {3, 2, 1}};
    int result[3][3];
    
    // 测试矩阵转置
    putch(84); // 'T'
    putch(10);
    transpose(mat1, result);
    print_matrix(result);
    
    // 测试矩阵乘法
    putch(77); // 'M'
    putch(10);
    matrix_mult(mat1, mat2, result);
    print_matrix(result);
    
    // 测试shadow
    {
        int mat1[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
        transpose(mat1, result);
        putch(83); // 'S'
        putch(10);
        print_matrix(result);
    }
    
    return 0;
}