/*
 * 测试多维数组、递归函数、短路逻辑和变量shadow
 * 功能：实现矩阵转置和乘法运算
 */

// 转置矩阵
void transpose(int rows, int cols, int src[][3], int dst[][2]) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < cols) {
            dst[j][i] = src[i][j];
            j = j + 1;
        }
        i = i + 1;
    }
}

// 矩阵乘法
void matrix_multiply(int a[][3], int b[][2], int result[][2], int a_rows, int a_cols, int b_cols) {
    int i = 0;
    while (i < a_rows) {
        int j = 0;
        while (j < b_cols) {
            result[i][j] = 0;
            int k = 0;
            while (k < a_cols) {
                result[i][j] = result[i][j] + a[i][k] * b[k][j];
                k = k + 1;
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印矩阵
void print_matrix(int rows, int cols, int mat[][2]) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < cols) {
            putint(mat[i][j]);
            if (j != cols - 1) {
                putch(32); // 空格
            }
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    // 初始化2x3矩阵
    int mat_a[2][3] = {{1, 2, 3}, {4, 5, 6}};
    
    // 测试shadow
    {
        int i = 999;
        putint(i); // 输出999
        putch(10);
    }
    
    // 转置矩阵
    int mat_b[3][2];
    transpose(2, 3, mat_a, mat_b);
    
    // 测试短路逻辑
    if (mat_b[0][0] == 1 && mat_b[1][0] == 2) {
        // 矩阵乘法
        int result[2][2];
        matrix_multiply(mat_a, mat_b, result, 2, 3, 2);
        
        // 输出结果
        print_matrix(2, 2, result);
    }
    
    return 0;
}