/*
 * 测试多维数组、函数递归、短路求值和变量shadow
 * 功能：计算并输出杨辉三角前N行
 */

// 递归计算杨辉三角的值
int yanghui(int row, int col) {
    if (col == 0 || col == row) {
        return 1;
    }
    return yanghui(row - 1, col - 1) + yanghui(row - 1, col);
}

// 初始化杨辉三角数组
void init_yanghui(int arr[][10], int rows) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j <= i) {
            arr[i][j] = yanghui(i, j);
            j = j + 1;
        }
        i = i + 1;
    }
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
    init_yanghui(triangle, n);
    
    // 输出杨辉三角
    int i = 0;
    while (i < n) {
        int j = 0;
        while (j <= i) {
            putint(triangle[i][j]);
            putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        
        // 测试shadow
        {
            int i = 999;
            putint(i); // 应输出999
            putch(10);
        }
        
        i = i + 1;
    }
    
    return 0;
}