/*
 * 测试多维数组、递归、短路求值和变量shadow
 * 功能：实现矩阵乘法并验证结果
 */

// 矩阵乘法函数
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
            if (j != size - 1) {
                putch(32); // 空格
            }
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

// 初始化矩阵
void init_matrix(int mat[][3], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            mat[i][j] = getint();
            j = j + 1;
        }
        i = i + 1;
    }
}

int main() {
    const int size = 3;
    int a[3][3];
    int b[3][3];
    int result[3][3];
    
    // 测试shadow
    {
        int size = 2; // shadow外层size
        int test_mat[2][2] = {{1, 2}, {3, 4}};
        putint(test_mat[0][0]);
        putch(10);
    }
    
    // 初始化矩阵A
    init_matrix(a, size);
    
    // 初始化矩阵B
    init_matrix(b, size);
    
    // 矩阵乘法
    matrix_multiply(a, b, result, size);
    
    // 输出结果
    print_matrix(result, size);
    
    // 测试短路求值
    if (size > 0 && result[0][0] > 0) {
        putch(84); // 'T'
    } else {
        putch(70); // 'F'
    }
    
    return 0;
}