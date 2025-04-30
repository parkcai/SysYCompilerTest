/*
 * 测试一维数组、多维数组、逻辑运算短路和变量shadow
 * 功能：实现一个简单的矩阵乘法，并验证结果
 */

// 初始化二维数组
void init_2d_array(int arr[][5], int rows, int cols) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < cols) {
            arr[i][j] = i * cols + j;
            j = j + 1;
        }
        i = i + 1;
    }
}

// 矩阵乘法
void matrix_multiply(int A[][5], int B[][5], int C[][5], int rows, int cols) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < cols) {
            C[i][j] = 0;
            int k = 0;
            while (k < cols) {
                C[i][j] = C[i][j] + A[i][k] * B[k][j];
                k = k + 1;
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印二维数组
void print_2d_array(int arr[][5], int rows, int cols) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < cols) {
            putint(arr[i][j]);
            putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    int A[5][5];
    int B[5][5];
    int C[5][5];

    // 初始化矩阵A和B
    init_2d_array(A, 5, 5);
    init_2d_array(B, 5, 5);

    // 计算矩阵乘法
    matrix_multiply(A, B, C, 5, 5);

    // 打印矩阵C
    print_2d_array(C, 5, 5);

    // 测试变量shadow
    {
        int i = 999;
        putint(i); // 应输出999
        putch(10); // 换行
    }

    return 0;
}