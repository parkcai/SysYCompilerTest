/*
 * 测试多维数组、逻辑运算、变量shadow和函数调用
 * 功能：实现一个简单的二维数组转置，并检查转置后的数组是否正确
 */

// 转置二维数组
void transpose_matrix(int a[][3], int b[][3], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            b[j][i] = a[i][j];
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印二维数组
void print_matrix(int mat[][3], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            putint(mat[i][j]);
            if (j != size - 1) {
                putch(32); // 空格
            }
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

// 检查两个二维数组是否相等
int check_matrices_equal(int a[][3], int b[][3], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            if (a[i][j] != b[i][j]) {
                return 0; // 不相等
            }
            j = j + 1;
        }
        i = i + 1;
    }
    return 1; // 相等
}

int main() {
    // 初始化一个3x3矩阵
    int matrix[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    int transposed[3][3];

    // 测试变量shadow
    {
        int matrix = 0;
        while (matrix < 3) {
            putint(matrix); // 输出0,1,2
            putch(10);
            matrix = matrix + 1;
        }
    }

    // 转置矩阵
    transpose_matrix(matrix, transposed, 3);

    // 打印原始矩阵
    putch(65); // 'A' 表示原始矩阵
    putch(10);
    print_matrix(matrix, 3);

    // 打印转置后的矩阵
    putch(66); // 'B' 表示转置后的矩阵
    putch(10);
    print_matrix(transposed, 3);

    // 检查转置是否正确
    int result = check_matrices_equal(matrix, transposed, 3);
    if (result == 0) {
        putch(84); // 'T' 表示转置正确
    } else {
        putch(70); // 'F' 表示转置错误
    }
    putch(10);

    return 0;
}