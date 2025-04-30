/*
 * 测试多维数组、函数递归、短路求值、变量shadow和库函数使用
 * 功能：实现矩阵转置并验证结果
 */

// 矩阵转置函数
void transpose(int mat[][3], int result[][3], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            result[j][i] = mat[i][j];
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印矩阵
void print_matrix(int mat[][3], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            putint(mat[i][j]);
            if (j < size - 1) {
                putch(32); // 空格
            }
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    // 定义原始矩阵
    int original[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    int transposed[3][3];
    
    // 测试shadow变量
    {
        int original = 100;
        putint(original); // 应输出100
        putch(10);
    }
    
    // 矩阵转置
    transpose(original, transposed, 3);
    
    // 测试短路求值
    if (transposed[0][0] == 1 && transposed[1][0] == 2) {
        print_matrix(transposed, 3);
    } else {
        putch(33); // 输出'!'表示异常
    }
    
    // 测试数组输入输出
    int arr[5];
    int n = getarray(arr);
    putarray(n, arr);
    
    return 0;
}