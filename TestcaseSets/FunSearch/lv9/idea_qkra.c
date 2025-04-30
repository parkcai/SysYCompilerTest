/*
 * 测试多维数组初始化、递归函数、逻辑运算和变量shadow
 * 功能：计算并输出矩阵乘法结果
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
void print_matrix(int mat[][3], int rows, int cols) {
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
    int a[3][3];
    int b[3][3];
    int result[3][3];
    
    // 初始化矩阵
    init_matrix(a, 3);
    init_matrix(b, 3);
    
    // 测试shadow变量
    {
        int a[2][2] = {{1,2},{3,4}};
        putarray(2, a[0]);
        putarray(2, a[1]);
    }
    
    // 矩阵乘法
    matrix_multiply(a, b, result, 3);
    
    // 输出结果
    print_matrix(result, 3, 3);
    
    return 0;
}