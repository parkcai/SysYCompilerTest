/*
 * 测试多维数组操作、函数递归、逻辑运算和变量shadow
 * 功能：实现矩阵转置并验证结果
 */

// 打印矩阵
void print_matrix(int mat[][3], int rows) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < 3) {
            putint(mat[i][j]);
            if (j < 2) {
                putch(32); // 空格
            }
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

// 矩阵转置
void transpose(int mat[][3], int rows) {
    int i = 0;
    while (i < rows) {
        int j = i + 1;
        while (j < 3) {
            // 交换元素
            int temp = mat[i][j];
            mat[i][j] = mat[j][i];
            mat[j][i] = temp;
            j = j + 1;
        }
        i = i + 1;
    }
}

int main() {
    // 初始化3x3矩阵
    int matrix[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    
    // 测试shadow
    {
        int matrix = 123;
        putint(matrix); // 应输出123
        putch(10);
    }
    
    // 转置前输出
    print_matrix(matrix, 3);
    
    // 执行转置
    transpose(matrix, 3);
    
    // 转置后输出
    print_matrix(matrix, 3);
    
    // 测试短路求值
    if (matrix[0][0] == 1 || matrix[1][1] != 5) {
        putch(84); // 'T'
    } else {
        putch(70); // 'F'
    }
    putch(10);
    
    return 0;
}