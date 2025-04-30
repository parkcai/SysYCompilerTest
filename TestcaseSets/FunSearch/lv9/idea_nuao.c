/*
 * 测试多维数组操作、函数递归调用、逻辑运算短路特性
 * 功能：计算并验证矩阵的转置和对角线元素
 */

// 递归计算斐波那契数列
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n-1) + fibonacci(n-2);
}

// 矩阵转置
void matrix_transpose(int a[][3], int b[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            b[j][i] = a[i][j];
            j = j + 1;
        }
        i = i + 1;
    }
}

// 验证矩阵对角线元素是否为斐波那契数
int check_diagonal_fib(int mat[][3]) {
    int cnt = 0;
    int i = 0;
    while (i < 3) {
        // 短路测试：当mat[i][i]<0时跳过判断
        if (mat[i][i] >= 0 && (mat[i][i] == fibonacci(mat[i][i]))) {
            cnt = cnt + 1;
        } else {
            putint(mat[i][i]); // 输出非斐波那契数
            putch(32); // 空格
        }
        i = i + 1;
    }
    return cnt;
}

int main() {
    // 定义矩阵
    int a[3][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}};
    int b[3][3];

    // 转置矩阵
    matrix_transpose(a, b);

    // 验证转置后的矩阵
    putint(b[0][0]); // 应输出0
    putch(32);
    putint(b[1][0]); // 应输出1
    putch(32);
    putint(b[2][0]); // 应输出2
    putch(10);

    // 检查对角线元素
    int result = check_diagonal_fib(b);
    putint(result); // 输出对角线上的斐波那契数个数

    return 0;
}