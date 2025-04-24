/*
 * 测试多维数组、函数递归和逻辑运算
 * 功能：实现矩阵乘法并验证结果
 */

// 矩阵乘法函数
void matrix_multiply(int m, int n, int p, int A[], int B[], int C[]) {
    int i = 0;
    while (i < m) {
        int j = 0;
        while (j < p) {
            C[i * p + j] = 0;
            int k = 0;
            while (k < n) {
                C[i * p + j] = C[i * p + j] + A[i * n + k] * B[k * p + j];
                k = k + 1;
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印矩阵
void print_matrix(int rows, int cols, int matrix[]) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < cols) {
            putint(matrix[i * cols + j]);
            if (j < cols - 1) {
                putch(32); // 空格
            }
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    // 测试矩阵A: 2x3
    int A[6] = {1, 2, 3, 4, 5, 6};
    // 测试矩阵B: 3x2
    int B[6] = {7, 8, 9, 10, 11, 12};
    // 结果矩阵C: 2x2
    int C[4];
    
    // 执行矩阵乘法
    matrix_multiply(2, 3, 2, A, B, C);
    
    // 输出结果
    print_matrix(2, 2, C);
    
    // 测试shadow变量
    {
        int A[4] = {1, 0, 0, 1}; // 单位矩阵
        int B[4] = {2, 0, 0, 2};
        int C[4];
        matrix_multiply(2, 2, 2, A, B, C);
        print_matrix(2, 2, C); // 应输出2 0 0 2
    }
    
    return 0;
}