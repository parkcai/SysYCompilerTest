/*
 * 测试多维数组、函数递归、逻辑运算和变量shadow
 * 功能：计算并输出一个矩阵的转置，并验证结果
 */

// 计算矩阵的转置
void transpose_matrix(int rows, int cols, int matrix[][10], int transposed[][10]) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < cols) {
            transposed[j][i] = matrix[i][j];
            j = j + 1;
        }
        i = i + 1;
    }
}

// 输出矩阵
void print_matrix(int rows, int cols, int matrix[][10]) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < cols) {
            putint(matrix[i][j]);
            if (j != cols - 1) {
                putch(32); // 空格
            }
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    int rows = getint(); // 获取行数
    int cols = getint(); // 获取列数
    
    // 测试短路求值
    if (rows <= 0 || cols <= 0 || rows > 10 || cols > 10) {
        putch(69); // 'E'
        putch(114); // 'r'
        putch(114); // 'r'
        putch(111); // 'o'
        putch(114); // 'r'
        return 0;
    }
    
    int matrix[10][10];
    int transposed[10][10];
    
    // 读取矩阵
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < cols) {
            matrix[i][j] = getint();
            j = j + 1;
        }
        i = i + 1;
    }
    
    // 计算转置
    transpose_matrix(rows, cols, matrix, transposed);
    
    // 输出原始矩阵
    putch(84); // 'T'
    putch(111); // 'o'
    putch(114); // 'r'
    putch(105); // 'i'
    putch(103); // 'g'
    putch(105); // 'i'
    putch(110); // 'n'
    putch(97);  // 'a'
    putch(108); // 'l'
    putch(32);  // 空格
    putch(77);  // 'M'
    putch(97);  // 'a'
    putch(116); // 't'
    putch(114); // 'r'
    putch(105); // 'i'
    putch(120); // 'x'
    putch(58);  // ':'
    putch(10);  // 换行
    print_matrix(rows, cols, matrix);
    
    // 输出转置矩阵
    putch(84); // 'T'
    putch(114); // 'r'
    putch(97);  // 'a'
    putch(110); // 'n'
    putch(115); // 's'
    putch(112); // 'p'
    putch(111); // 'o'
    putch(115); // 's'
    putch(101); // 'e'
    putch(100); // 'd'
    putch(32);  // 空格
    putch(77);  // 'M'
    putch(97);  // 'a'
    putch(116); // 't'
    putch(114); // 'r'
    putch(105); // 'i'
    putch(120); // 'x'
    putch(58);  // ':'
    putch(10);  // 换行
    print_matrix(cols, rows, transposed);
    
    // 测试变量shadow
    {
        int i = 5;
        int j = 5;
        putint(transposed[i][j]);  // 输出第6行第6列的元素
        putch(10);                 // 换行
    }
    
    return 0;
}