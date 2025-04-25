/*
 * 测试多维数组初始化、函数递归和逻辑运算
 * 功能：实现矩阵转置并验证结果
 */

// 初始化矩阵
void init_matrix(int matrix[][3], int rows) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < 3) {
            matrix[i][j] = i * 3 + j + 1; // 填充1~9
            j = j + 1;
        }
        i = i + 1;
    }
}

// 转置矩阵
void transpose(int src[][3], int dst[][3], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < 3) {
            dst[j][i] = src[i][j]; // 测试多维数组访问
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印矩阵
void print_matrix(int matrix[][3], int rows) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < 3) {
            putint(matrix[i][j]);
            if (j < 2) {
                putch(32); // 空格
            }
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    int original[3][3];
    int transposed[3][3];
    
    // 初始化原始矩阵
    init_matrix(original, 3);
    
    // 测试短路求值
    if (original[0][0] == 1 || original[1][1] != 5) {
        putch(33); // 输出'!'
    }
    
    // 转置矩阵
    transpose(original, transposed, 3);
    
    // 输出结果
    putch(79); // 'O'
    putch(58); // ':'
    putch(10);
    print_matrix(original, 3);
    
    putch(84); // 'T'
    putch(58); // ':'
    putch(10);
    print_matrix(transposed, 3);
    
    // 测试shadow
    {
        int original[2][3] = {{1, 2, 3}, {4, 5, 6}};
        putch(83); // 'S'
        putch(58); // ':'
        putch(10);
        print_matrix(original, 2);
    }
    
    return 0;
}