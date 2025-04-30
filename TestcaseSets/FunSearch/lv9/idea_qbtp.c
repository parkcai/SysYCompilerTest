/*
 * 测试多维数组、递归、短路求值和变量shadow
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
void print_matrix(int m[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            putint(m[i][j]);
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
    // 定义两个3x3矩阵
    int a[3][3] = {
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
    };
    int b[3][3] = {
        {9, 8, 7},
        {6, 5, 4},
        {3, 2, 1}
    };
    int result[3][3];
    
    // 测试shadow
    {
        int a = 123;
        putint(a); // 应输出123
        putch(10);
    }
    
    // 计算矩阵乘法
    matrix_multiply(a, b, result);
    
    // 打印结果矩阵
    print_matrix(result);
    
    // 验证结果是否正确
    int expected[3][3] = {
        {30, 24, 18},
        {84, 69, 54},
        {138, 114, 90}
    };
    
    // 测试短路求值
    if (matrix_equal(result, expected) || getint() == 1) {
        putch(84); // 'T'
    } else {
        putch(70); // 'F'
    }
    putch(10);
    
    return 0;
}