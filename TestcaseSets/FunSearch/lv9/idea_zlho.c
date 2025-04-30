/*
 * 测试多维数组、递归函数、短路求值和变量shadow
 * 功能：计算并输出杨辉三角的前n行
 */

// 计算杨辉三角的值
void pascal_triangle(int rows) {
    int triangle[10][10];
    int i = 0;
    
    // 初始化第一列和对角线
    while (i < rows) {
        triangle[i][0] = 1;
        triangle[i][i] = 1;
        i = i + 1;
    }
    
    // 计算内部元素
    i = 2;
    while (i < rows) {
        int j = 1;
        while (j < i) {
            triangle[i][j] = triangle[i-1][j-1] + triangle[i-1][j];
            j = j + 1;
        }
        i = i + 1;
    }
    
    // 输出杨辉三角
    i = 0;
    while (i < rows) {
        int j = 0;
        while (j <= i) {
            putint(triangle[i][j]);
            putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        
        // 测试shadow
        {
            int i = 99;
            if (i > 50 || rows < 0) { // 测试短路求值
                putint(i); // 应输出99
                putch(10);
            }
        }
        i = i + 1;
    }
}

int main() {
    int n = getint(); // 获取要输出的行数
    
    // 测试短路求值
    if (n <= 0 || n > 10) {
        putch(69); // 'E'
        putch(114); // 'r'
        putch(114); // 'r'
        putch(111); // 'o'
        putch(114); // 'r'
        return 0;
    }
    
    pascal_triangle(n);
    
    // 测试多维数组访问
    int arr[2][3] = {{1, 2, 3}, {4, 5, 6}};
    putint(arr[1][2]); // 应输出6
    return 0;
}