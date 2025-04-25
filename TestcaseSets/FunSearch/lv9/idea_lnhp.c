/*
 * 测试多维数组初始化、函数递归、逻辑运算短路和变量shadow
 * 功能：计算并输出矩阵乘法结果
 */

// 递归初始化矩阵
void init_matrix(int mat[][3], int size, int row, int col, int val) {
    if (row >= size) return;
    if (col >= size) {
        init_matrix(mat, size, row + 1, 0, val + 1);
        return;
    }
    mat[row][col] = val;
    init_matrix(mat, size, row, col + 1, val + 1);
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
            putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    const int SIZE = 3;
    int mat1[3][3];
    int mat2[3][3];
    int result[3][3];
    
    // 测试shadow
    {
        int SIZE = 2;
        putint(SIZE); // 应输出2而非3
        putch(10);
    }
    
    // 初始化矩阵
    init_matrix(mat1, SIZE, 0, 0, 1);
    init_matrix(mat2, SIZE, 0, 0, 9);
    
    // 矩阵乘法
    matrix_multiply(mat1, mat2, result, SIZE);
    
    // 输出结果
    print_matrix(result, SIZE);
    
    return 0;
}