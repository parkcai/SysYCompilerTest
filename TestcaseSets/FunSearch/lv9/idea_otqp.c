/*
 * 测试多维数组操作和逻辑运算
 * 包含变量shadow和库函数使用
 */

const int DIM = 3;  // 定义常量维度

// 计算矩阵对角线元素和
int diagonal_sum(int mat[][3]) {
    int sum = 0;
    int i = 0;
    while (i < DIM) {
        sum = sum + mat[i][i];
        i = i + 1;
    }
    return sum;
}

// 判断是否为幻方
int is_magic_square(int mat[][3]) {
    int target = diagonal_sum(mat);
    int row_sum = 0;
    int col_sum = 0;
    int i = 0;
    
    // 检查行和列
    while (i < DIM) {
        row_sum = 0;
        col_sum = 0;
        int j = 0;
        while (j < DIM) {
            row_sum = row_sum + mat[i][j];
            col_sum = col_sum + mat[j][i];
            j = j + 1;
        }
        if (row_sum != target || col_sum != target) {
            return 0;  // 不是幻方
        }
        i = i + 1;
    }
    return 1;  // 是幻方
}

int main() {
    int matrix[3][3];
    int i = 0;
    
    // 读取矩阵
    while (i < DIM) {
        int j = 0;
        while (j < DIM) {
            matrix[i][j] = getint();
            j = j + 1;
        }
        i = i + 1;
    }
    
    // shadow测试
    {
        int DIM = 2;  // shadow全局常量
        putint(DIM);  // 输出2
        putch(10);    // 换行
    }
    
    // 逻辑运算测试
    if (is_magic_square(matrix) && diagonal_sum(matrix) > 10) {
        putint(1);  // 是幻方且对角线和大10
    } else {
        putint(0);  // 不是
    }
    
    return 0;
}