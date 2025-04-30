/*
 * 测试多维数组、递归函数、逻辑运算短路和变量shadow
 * 功能：实现一个简单的矩阵乘法并输出结果
 */

// 计算两个矩阵的乘积
void matrix_multiply(int A[][3], int B[][3], int C[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            C[i][j] = 0;
            int k = 0;
            while (k < 3) {
                C[i][j] = C[i][j] + A[i][k] * B[k][j];
                k = k + 1;
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印矩阵
void print_matrix(int M[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            putint(M[i][j]);
            if (j < 2) putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    // 初始化矩阵
    int A[3][3] = {
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
    };
    
    int B[3][3] = {
        {9, 8, 7},
        {6, 5, 4},
        {3, 2, 1}
    };
    
    int C[3][3]; // 存储结果的矩阵
    
    // 计算矩阵乘积
    matrix_multiply(A, B, C);
    
    // 打印结果矩阵
    print_matrix(C);
    
    // 测试变量shadow
    {
        int A = 123;
        putint(A); // 应输出123
        putch(10);
    }
    
    return 0;
}