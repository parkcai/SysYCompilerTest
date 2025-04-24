/*
 * 测试多维数组、函数调用、变量shadow和逻辑运算
 * 功能：实现矩阵乘法并验证结果
 */

// 矩阵乘法函数
void matrix_mult(int a[][3], int b[][3], int result[][3], int size) {
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

// 验证矩阵乘法结果
int verify_result(int result[][3], int size) {
    int expected[3][3] = {
        {30, 36, 42},
        {66, 81, 96},
        {102, 126, 150}
    };
    
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            if (result[i][j] != expected[i][j]) {
                return 0;
            }
            j = j + 1;
        }
        i = i + 1;
    }
    return 1;
}

int main() {
    // 定义两个3x3矩阵
    int a[3][3] = {
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
    };
    
    int b[3][3] = {
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
    };
    
    int result[3][3] = {0};
    
    // 计算矩阵乘法
    matrix_mult(a, b, result, 3);
    
    // 测试shadow变量
    {
        int a = 123;
        int b = 456;
        putint(a + b); // 应输出579
        putch(10);
    }
    
    // 打印结果矩阵
    print_matrix(result, 3);
    
    // 验证结果
    if (verify_result(result, 3)) {
        putch(79); // 'O'
        putch(75); // 'K'
    } else {
        putch(69); // 'E'
        putch(82); // 'R'
        putch(82); // 'R'
    }
    
    return 0;
}