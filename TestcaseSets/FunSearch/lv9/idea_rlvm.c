/*
 * 测试多维数组、递归、逻辑运算短路和变量shadow
 * 功能：实现矩阵乘法并验证结果
 */

// 递归计算矩阵乘法的一个元素
int calc_element(int a[], int b[], int row, int col, int n, int k) {
    if (k == 0) return 0;
    int a_idx = row * n + (k - 1);
    int b_idx = (k - 1) * n + col;
    return a[a_idx] * b[b_idx] + calc_element(a, b, row, col, n, k - 1);
}

// 矩阵乘法函数
void matrix_multiply(int a[], int b[], int result[], int n) {
    int i = 0;
    while (i < n) {
        int j = 0;
        while (j < n) {
            // 使用递归计算每个元素
            result[i * n + j] = calc_element(a, b, i, j, n, n);
            
            // 测试短路求值
            if (i == 0 && j == 0 || result[i * n + j] > 100) {
                putint(result[i * n + j]); // 输出第一个元素或大于100的元素
                putch(32); // 空格
            }
            
            j = j + 1;
        }
        i = i + 1;
    }
}

int main() {
    const int size = 2; // 矩阵大小
    
    // 定义两个2x2矩阵
    int mat1[4] = {1, 2, 3, 4};
    int mat2[4] = {5, 6, 7, 8};
    int result[4] = {0};
    
    // 测试shadow
    {
        int size = 3; // shadow常量size
        putint(size); // 应输出3
        putch(10); // 换行
    }
    
    // 执行矩阵乘法
    matrix_multiply(mat1, mat2, result, size);
    
    // 输出结果矩阵
    putch(10); // 换行
    putarray(4, result);
    
    return 0;
}