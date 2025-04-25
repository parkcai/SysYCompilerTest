/*
 * 测试多维数组操作、函数递归和逻辑运算的综合程序
 * 功能：实现矩阵转置并验证结果
 */

// 初始化矩阵
void init_matrix(int mat[][3], int rows) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < 3) {
            mat[i][j] = i * 3 + j + 1;
            j = j + 1;
        }
        i = i + 1;
    }
}

// 转置矩阵
void transpose(int mat[][3], int result[][3], int rows) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < 3) {
            result[j][i] = mat[i][j];
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

int main() {
    // 测试二维数组
    int matrix[3][3];
    int transposed[3][3];
    
    // 初始化并打印原始矩阵
    init_matrix(matrix, 3);
    putch(79); // 'O'
    putch(114); // 'r'
    putch(105); // 'i'
    putch(103); // 'g'
    putch(105); // 'i'
    putch(110); // 'n'
    putch(97); // 'a'
    putch(108); // 'l'
    putch(58); // ':'
    putch(10); // 换行
    print_matrix(matrix, 3);
    
    // 转置并打印结果
    transpose(matrix, transposed, 3);
    putch(84); // 'T'
    putch(114); // 'r'
    putch(97); // 'a'
    putch(110); // 'n'
    putch(115); // 's'
    putch(112); // 'p'
    putch(111); // 'o'
    putch(115); // 's'
    putch(101); // 'e'
    putch(100); // 'd'
    putch(58); // ':'
    putch(10); // 换行
    print_matrix(transposed, 3);
    
    // 测试短路求值
    if (matrix[0][0] == 1 && matrix[2][2] == 9 || matrix[1][1] == 5) {
        putch(86); // 'V'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        putch(10); // 换行
    }
    
    return 0;
}