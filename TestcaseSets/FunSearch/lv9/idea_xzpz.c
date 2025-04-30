/*
 * 测试递归函数、数组操作、短路求值和变量shadow
 * 功能：计算斐波那契数列并验证矩阵运算
 */

// 递归计算斐波那契数列
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n-1) + fibonacci(n-2);
}

// 矩阵加法
void matrix_add(int a[][2], int b[][2], int result[][2]) {
    int i = 0;
    while (i < 2) {
        int j = 0;
        while (j < 2) {
            result[i][j] = a[i][j] + b[i][j];
            j = j + 1;
        }
        i = i + 1;
    }
}

// 验证矩阵运算
void verify_matrix(int mat[][2]) {
    // 测试短路求值
    if (mat[0][0] > 0 || (mat[1][1] / mat[0][0] > 0)) {
        putint(1); // 不会执行除法
    } else {
        putint(0);
    }
}

int main() {
    // 计算斐波那契数列
    int fib[10];
    int i = 0;
    while (i < 10) {
        fib[i] = fibonacci(i);
        i = i + 1;
    }
    
    // 测试shadow
    {
        int i = 5;
        putint(fib[i]); // 应输出fib[5]=5
        putch(10);
    }
    
    // 矩阵运算测试
    int a[2][2] = {{1, 2}, {3, 4}};
    int b[2][2] = {{5, 6}, {7, 8}};
    int result[2][2];
    
    matrix_add(a, b, result);
    verify_matrix(result);
    
    // 输出斐波那契数列
    putch(10);
    i = 0;
    while (i < 10) {
        putint(fib[i]);
        putch(32); // 空格
        i = i + 1;
    }
    
    return 0;
}