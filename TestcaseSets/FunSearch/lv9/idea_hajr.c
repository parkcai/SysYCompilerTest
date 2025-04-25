/*
 * 测试多维数组初始化、函数递归和逻辑运算
 * 功能：实现矩阵转置并验证结果
 */

// 递归转置矩阵
void transpose(int size, int mat[][10], int row, int col) {
    if (row >= size) return;
    
    if (col >= size) {
        transpose(size, mat, row + 1, 0);
        return;
    }
    
    // 只处理上三角部分
    if (row < col) {
        int temp = mat[row][col];
        mat[row][col] = mat[col][row];
        mat[col][row] = temp;
    }
    
    transpose(size, mat, row, col + 1);
}

// 打印矩阵
void print_matrix(int size, int mat[][10]) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            putint(mat[i][j]);
            if (j != size - 1) putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    const int SIZE = 3;
    int matrix[10][10] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    
    // 测试短路求值
    if (SIZE > 0 && matrix[0][0] == 1) {
        putch(84); // 'T'
    } else {
        putch(70); // 'F'
    }
    
    // 转置前
    print_matrix(SIZE, matrix);
    putch(10);
    
    // 转置操作
    transpose(SIZE, matrix, 0, 0);
    
    // 转置后
    print_matrix(SIZE, matrix);
    
    // 测试shadow
    {
        int SIZE = 2;
        int matrix[10][10] = {{10, 20}, {30, 40}};
        transpose(SIZE, matrix, 0, 0);
        putch(10);
        print_matrix(SIZE, matrix);
    }
    
    return 0;
}