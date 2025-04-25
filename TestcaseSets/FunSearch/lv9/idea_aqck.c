/*
 * 测试多维数组操作、递归函数和复杂逻辑表达式
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

// 递归转置矩阵
void transpose(int mat[][3], int result[][3], int row, int col) {
    if (row >= 3 || col >= 3) {
        return;
    }
    
    result[col][row] = mat[row][col];
    
    if (col + 1 < 3) {
        transpose(mat, result, row, col + 1);
    } else {
        transpose(mat, result, row + 1, 0);
    }
}

// 打印矩阵
void print_matrix(int mat[][3], int rows) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < 3) {
            putint(mat[i][j]);
            putch(32);
            j = j + 1;
        }
        putch(10);
        i = i + 1;
    }
}

int main() {
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
    putch(10);
    print_matrix(matrix, 3);
    
    // 转置矩阵
    transpose(matrix, transposed, 0, 0);
    
    // 打印转置结果
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
    putch(10);
    print_matrix(transposed, 3);
    
    // 测试短路逻辑
    if (matrix[0][0] == 1 && transposed[0][0] == 1 || matrix[1][1] != 5) {
        putch(33); // '!'
        putch(10);
    }
    
    return 0;
}