/*
 * 测试多维数组初始化、逻辑运算短路特性、函数递归和变量shadow
 * 功能：计算两个矩阵的Hadamard积（逐元素乘积）并输出结果
 */

// 递归初始化矩阵
void init_matrix(int mat[][3], int size, int val) {
    if (size <= 0) return;
    int i = 0;
    while (i < 3) {
        mat[size-1][i] = val + i;
        i = i + 1;
    }
    init_matrix(mat, size-1, val+3);
}

// 计算Hadamard积
void hadamard_product(int a[][3], int b[][3], int result[][3], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < 3) {
            // 利用短路特性防止除零错误
            if (b[i][j] != 0 && (a[i][j] / b[i][j] > 0)) {
                result[i][j] = a[i][j] * b[i][j];
            } else {
                result[i][j] = 0;
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印矩阵
void print_matrix(int mat[][3], int rows) {
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

int main() {
    int mat1[3][3];
    int mat2[3][3];
    int result[3][3];
    
    // 初始化矩阵
    init_matrix(mat1, 3, 1);
    init_matrix(mat2, 3, 2);
    
    // 计算Hadamard积
    hadamard_product(mat1, mat2, result, 3);
    
    // 测试shadow变量
    {
        int i = 0;
        while (i < 3) {
            int j = 0;
            while (j < 3) {
                result[i][j] = result[i][j] + 1; // 每个元素加1
                j = j + 1;
            }
            i = i + 1;
        }
    }
    
    // 输出结果
    print_matrix(result, 3);
    
    return 0;
}