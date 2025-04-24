/*
 * 测试多维数组、递归函数、短路求值和变量shadow
 * 功能：计算并输出杨辉三角的前n行
 */

// 计算杨辉三角的值
int pascal(int row, int col) {
    if (col == 0 || col == row) {
        return 1;
    }
    return pascal(row - 1, col - 1) + pascal(row - 1, col);
}

int main() {
    int n = getint(); // 获取要输出的行数
    
    // 测试短路求值
    if (n <= 0 || n > 10) {
        putint(-1); // 输入不合法
        return 0;
    }
    
    // 定义二维数组存储杨辉三角
    int triangle[10][10];
    int i = 0;
    while (i < n) {
        int j = 0;
        while (j <= i) {
            triangle[i][j] = pascal(i, j);
            
            // 输出当前元素
            putint(triangle[i][j]);
            putch(32); // 空格
            
            // 测试shadow
            {
                int j = 100;
                if (j > 50 || i < 0) { // 测试短路求值
                    putint(j); // 应输出100
                    putch(32);
                }
            }
            
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
    
    // 测试多维数组访问
    putint(triangle[n-1][n/2]); // 输出最后一行中间元素
    return 0;
}