/*
 * 测试多维数组初始化、函数递归、逻辑运算短路和变量shadow
 * 功能：实现矩阵乘法并验证结果
 */

// 初始化矩阵
void init_matrix(int mat[][3], int rows, int start) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < 3) {
            mat[i][j] = start + i * 3 + j;
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

int main() {
    int mat_a[3][3];
    int mat_b[3][3];
    int result[3][3];
    
    // 初始化矩阵
    init_matrix(mat_a, 3, 1);
    init_matrix(mat_b, 3, 9);
    
    // 测试shadow变量
    {
        int mat_a[3][3] = {{0}}; // shadow全局mat_a
        print_matrix(mat_a, 3); // 应输出全0矩阵
    }
    
    // 矩阵乘法
    matrix_multiply(mat_a, mat_b, result, 3);
    
    // 输出结果
    print_matrix(result, 3);
    
    // 测试短路特性
    if (getint() != 0 || getint() != 0) {
        putch(84); // 'T'
    } else {
        putch(70); // 'F'
    }
    
    return 0;
}