/*
 * 测试多维数组、递归、短路求值和库函数使用
 * 功能：实现矩阵乘法并验证结果
 */

// 矩阵乘法函数
void matrix_multiply(int a[][3], int b[][3], int result[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            result[i][j] = 0;
            int k = 0;
            while (k < 3) {
                result[i][j] = result[i][j] + a[i][k] * b[k][j];
                k = k + 1;
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印矩阵
void print_matrix(int mat[][3]) {
    int i = 0;
    while (i < 3) {
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

// 验证矩阵相等
int matrix_equal(int a[][3], int b[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            if (a[i][j] != b[i][j]) {
                return 0;
            }
            j = j + 1;
        }
        i = i + 1;
    }
    return 1;
}

int main() {
    // 测试矩阵A
    int a[3][3] = {
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
    };
    
    // 测试矩阵B
    int b[3][3] = {
        {9, 8, 7},
        {6, 5, 4},
        {3, 2, 1}
    };
    
    // 结果矩阵
    int result[3][3] = {0};
    
    // 执行矩阵乘法
    matrix_multiply(a, b, result);
    
    // 打印结果
    print_matrix(result);
    
    // 验证结果
    int expected[3][3] = {
        {30, 24, 18},
        {84, 69, 54},
        {138, 114, 90}
    };
    
    // 测试短路求值
    if (matrix_equal(result, expected) || getint() == 1) {
        putch(80); // 'P'
        putch(97); // 'a'
        putch(115); // 's'
        putch(115); // 's'
    } else {
        putch(70); // 'F'
        putch(97); // 'a'
        putch(105); // 'i'
        putch(108); // 'l'
    }
    putch(10);
    
    return 0;
}