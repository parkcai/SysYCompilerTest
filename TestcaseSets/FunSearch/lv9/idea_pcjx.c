/*
 * 测试多维数组初始化、函数递归、逻辑运算和库函数综合应用
 * 功能：计算并输出杨辉三角的前n行
 */

// 递归计算杨辉三角的值
int pascal(int row, int col) {
    if (col == 0 || col == row) {
        return 1;
    }
    return pascal(row - 1, col - 1) + pascal(row - 1, col);
}

int main() {
    int n = getint(); // 获取要输出的行数
    
    // 合法性检查
    if (n <= 0 || n > 10) {
        putch(33); // 输出'!'表示错误
        return -1;
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
                int j = 99;
                if (i == 0 || j > 50) { // 测试短路求值
                    putint(j); // 应输出99
                    putch(10);
                }
            }
            
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
    
    // 测试数组访问
    putarray(n, triangle[n-1]); // 输出最后一行
    return 0;
}