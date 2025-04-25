/*
 * 测试多维数组、递归函数和逻辑运算短路
 * 功能：实现螺旋矩阵生成并验证结果
 */

// 递归生成螺旋矩阵
void spiral_matrix(int n, int mat[][10], int start, int end, int num) {
    if (start > end) return;
    
    // 填充上边
    int i = start;
    while (i <= end) {
        mat[start][i] = num;
        num = num + 1;
        i = i + 1;
    }
    
    // 填充右边
    i = start + 1;
    while (i <= end) {
        mat[i][end] = num;
        num = num + 1;
        i = i + 1;
    }
    
    // 填充下边
    if (start < end) {
        i = end - 1;
        while (i >= start) {
            mat[end][i] = num;
            num = num + 1;
            i = i - 1;
        }
    }
    
    // 填充左边
    if (start < end) {
        i = end - 1;
        while (i > start) {
            mat[i][start] = num;
            num = num + 1;
            i = i - 1;
        }
    }
    
    // 递归填充内层
    spiral_matrix(n, mat, start + 1, end - 1, num);
}

// 打印矩阵
void print_matrix(int n, int mat[][10]) {
    int i = 0;
    while (i < n) {
        int j = 0;
        while (j < n) {
            putint(mat[i][j]);
            if (j != n - 1) {
                putch(32); // 空格
            }
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    const int SIZE = 5;
    int matrix[10][10];
    
    // 生成螺旋矩阵
    spiral_matrix(SIZE, matrix, 0, SIZE - 1, 1);
    
    // 打印矩阵
    print_matrix(SIZE, matrix);
    
    // 测试短路逻辑和shadow变量
    {
        const int SIZE = 3;
        int small_mat[10][10];
        spiral_matrix(SIZE, small_mat, 0, SIZE - 1, 1);
        
        // 测试短路求值
        if (SIZE > 0 && small_mat[0][0] == 1) {
            putch(84); // 'T'
        } else {
            putch(70); // 'F'
        }
        putch(10);
        print_matrix(SIZE, small_mat);
    }
    
    return 0;
}