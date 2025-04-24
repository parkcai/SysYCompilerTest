/*
 * 矩阵乘法测试
 * 测试点：多维数组操作、函数参数传递、嵌套循环
 */

// 矩阵初始化
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
void multiply(int a[][3], int b[][3], int result[][3], int size) {
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
    int result[3][3];
    
    // 初始化矩阵
    init_matrix(mat1, 3);
    init_matrix(mat2, 3);
    
    // 矩阵乘法
    multiply(mat1, mat2, result, 3);
    
    // 输出结果
    print_matrix(result, 3, 3);
    
    // 测试shadow
    {
        int i = 42;
        putch(10);
        putint(i); // 应输出42
    }
    
    return 0;
}