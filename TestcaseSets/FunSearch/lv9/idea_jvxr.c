/*
 * 测试多维数组、递归函数、短路逻辑和变量shadow
 * 功能：计算并输出杨辉三角的前N行
 */

// 递归计算杨辉三角的值
int pascal_value(int row, int col) {
    if (col == 0 || col == row) {
        return 1;
    }
    return pascal_value(row - 1, col - 1) + pascal_value(row - 1, col);
}

// 初始化二维数组为杨辉三角
void init_pascal_triangle(int arr[][10], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j <= i) {
            arr[i][j] = pascal_value(i, j);
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印二维数组
void print_2d_array(int arr[][10], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j <= i) {
            putint(arr[i][j]);
            if (j < i) {
                putch(32); // 空格
            }
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    int n = getint(); // 获取杨辉三角行数
    
    // 测试输入合法性
    if (n <= 0 || n > 10) {
        putch(33); // 输出'!'表示错误
        return -1;
    }
    
    int triangle[10][10]; // 存储杨辉三角
    
    // 初始化数组
    init_pascal_triangle(triangle, n);
    
    // 输出结果
    print_2d_array(triangle, n);
    
    // 测试shadow变量和短路逻辑
    {
        int n = 3;
        if (n > 0 && (triangle[n-1][0] == 1)) {
            putch(10); // 换行
            putint(triangle[n-1][0]); // 应输出1
        }
    }
    
    return 0;
}