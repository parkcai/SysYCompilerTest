/*
 * 测试多维数组操作和嵌套作用域
 * 功能：矩阵转置并验证shadow行为
 */

// 矩阵转置函数
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
        int i = 0;
        while (i < 3) {
            putarray(3, matrix[i]);
            i = i + 1;
        }
        
        // shadow变量
        int matrix = 42;
        putint(matrix); // 应输出42
        putch(10);     // 换行
    }
    
    // 执行转置
    transpose(matrix, 3);
    
    // 输出转置结果
    int i = 0;
    while (i < 3) {
        putarray(3, matrix[i]);
        i = i + 1;
    }
    
    return 0;
}