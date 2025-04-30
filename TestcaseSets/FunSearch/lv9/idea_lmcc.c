/*
 * 测试递归函数、多维数组和逻辑运算
 * 功能：计算斐波那契数列并验证矩阵转置
 */

// 递归计算斐波那契数列
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 矩阵转置
void transpose(int mat[][4], int result[][4]) {
    int i = 0;
    while (i < 4) {
        int j = 0;
        while (j < 4) {
            result[j][i] = mat[i][j];
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印矩阵
void print_matrix(int mat[][4]) {
    int i = 0;
    while (i < 4) {
        int j = 0;
        while (j < 4) {
            putint(mat[i][j]);
            if (j < 3) {
                putch(32); // 空格
            }
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    // 测试斐波那契数列
    int n = getint();
    putint(fibonacci(n));
    putch(10);
    
    // 测试矩阵转置
    int mat[4][4] = {
        {1, 2, 3, 4},
        {5, 6, 7, 8},
        {9, 10, 11, 12},
        {13, 14, 15, 16}
    };
    int transposed[4][4];
    
    transpose(mat, transposed);
    print_matrix(transposed);
    
    // 测试短路逻辑
    if (n > 0 || fibonacci(10) == 55) {
        putch(84); // 'T'
    } else {
        putch(70); // 'F'
    }
    putch(10);
    
    return 0;
}