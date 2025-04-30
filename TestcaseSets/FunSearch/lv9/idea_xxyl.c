/*
 * 测试递归函数、多维数组和短路逻辑运算
 * 功能：计算斐波那契数列并验证矩阵转置操作
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
void print_matrix(int mat[][4], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            putint(mat[i][j]);
            if (j < size - 1) {
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
    
    // 测试短路逻辑
    if (n > 0 && fibonacci(n) % 2 == 0) {
        putch(69); // 'E'
    } else {
        putch(79); // 'O'
    }
    putch(10);
    
    // 测试矩阵转置
    int mat[4][4] = {{1,2,3,4},{5,6,7,8},{9,10,11,12},{13,14,15,16}};
    int result[4][4];
    
    transpose(mat, result);
    print_matrix(result, 4);
    
    // 测试shadow
    {
        int n = 5;
        putint(fibonacci(n));
        putch(10);
    }
    
    return 0;
}