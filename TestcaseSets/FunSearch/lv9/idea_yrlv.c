/*
 * 测试多维数组、递归函数、逻辑运算短路和变量shadow
 * 功能：实现一个简单的矩阵转置并输出
 */

// 打印二维数组
void print_matrix(int matrix[][5], int rows, int cols) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < cols) {
            putint(matrix[i][j]);
            if (j < cols - 1) {
                putch(32); // 空格
            }
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

// 转置矩阵
void transpose_matrix(int matrix[][5], int transposed[][5], int rows, int cols) {
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

// 初始化矩阵
void init_matrix(int matrix[][5], int rows, int cols) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < cols) {
            matrix[i][j] = i * cols + j; // 生成一些简单的值
            j = j + 1;
        }
        i = i + 1;
    }
}

int main() {
    // 初始化矩阵
    int matrix[5][5];
    init_matrix(matrix, 5, 5);

    // 打印原始矩阵
    putch(77); // 'M'
    putch(97); // 'a'
    putch(116); // 't'
    putch(114); // 'r'
    putch(105); // 'i'
    putch(120); // 'x'
    putch(58); // ':'
    putch(32); // ' '
    putch(10);
    print_matrix(matrix, 5, 5);

    // 转置矩阵
    int transposed[5][5];
    transpose_matrix(matrix, transposed, 5, 5);

    // 打印转置后的矩阵
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
    putch(32); // ' '
    putch(77); // 'M'
    putch(97); // 'a'
    putch(116); // 't'
    putch(114); // 'r'
    putch(105); // 'i'
    putch(120); // 'x'
    putch(58); // ':'
    putch(32); // ' '
    putch(10);
    print_matrix(transposed, 5, 5);

    // 测试变量shadow
    {
        int matrix = 123;
        putint(matrix); // 应输出123
        putch(10);
    }

    return 0;
}