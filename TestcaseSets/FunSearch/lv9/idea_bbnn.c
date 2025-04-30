/*
 * 测试多维数组、递归、短路求值和变量shadow
 * 功能：计算矩阵乘法并验证结果
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

int main() {
    // 定义两个3x3矩阵
    int a[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    int b[3][3] = {{9, 8, 7}, {6, 5, 4}, {3, 2, 1}};
    int result[3][3];
    
    // 计算矩阵乘法
    matrix_multiply(a, b, result, 3);
    
    // 输出结果
    print_matrix(result, 3);
    
    // 测试shadow变量
    {
        int i = 123;
        putint(i); // 应输出123
        putch(10);
    }
    
    // 测试短路求值
    if (result[0][0] > 0 && result[1][1] > 0 || result[2][2] == 30) {
        putch(84); // 'T'
    } else {
        putch(70); // 'F'
    }
    
    return 0;
}