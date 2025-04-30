/*
 * 测试用例：实现一个简单的矩阵转置程序
 * 功能：用户输入一个3x3的矩阵，程序计算并输出其转置矩阵。
 * 本测试用例旨在验证SysY语言中的多维数组操作、函数定义与调用、逻辑运算和变量shadow。
 */

// 打印二维数组
void print_2d_array(int mat[][3], int rows) {
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

// 计算矩阵的转置
void transpose(int mat[][3], int result[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            result[j][i] = mat[i][j];
            j = j + 1;
        }
        i = i + 1;
    }
}

int main() {
    // 定义一个3x3的矩阵
    int matrix[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    
    // 输入矩阵
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            matrix[i][j] = getint();
            j = j + 1;
        }
        i = i + 1;
    }

    // 初始化转置矩阵
    int transposed[3][3];

    // 计算转置矩阵
    transpose(matrix, transposed);

    // 输出原矩阵
    putch(77); // 'M'
    putch(58); // ':'
    putch(10); // 换行
    print_2d_array(matrix, 3);

    // 输出转置矩阵
    putch(84); // 'T'
    putch(58); // ':'
    putch(10); // 换行
    print_2d_array(transposed, 3);

    // 测试变量shadow
    {
        int i = 10;
        putint(i); // 应输出10
        putch(10);
    }

    return 0;
}