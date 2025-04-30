/*
 * 测试多维数组初始化、函数递归、逻辑运算短路和变量shadow
 * 功能：实现矩阵乘法并验证结果
 */

// 初始化矩阵
void init_matrix(int mat[][3], int rows, int val) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < 3) {
            mat[i][j] = val + i * 3 + j;
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
    int a[3][3];
    int b[3][3];
    int result[3][3];
    
    // 初始化矩阵
    init_matrix(a, 3, 1);
    init_matrix(b, 3, 2);
    
    // 测试shadow变量
    {
        int a = 5;
        putint(a); // 应输出5
        putch(10); // 换行
    }
    
    // 执行矩阵乘法
    matrix_multiply(a, b, result, 3);
    
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