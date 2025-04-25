/*
 * 测试多维数组、函数递归、逻辑运算和变量shadow
 * 功能：计算并输出杨辉三角的前n行
 */

// 计算杨辉三角的值
int pascal(int row, int col, int memo[][10]) {
    if (col == 0 || col == row) {
        memo[row][col] = 1;
        return 1;
    }
    
    // 利用memo数组避免重复计算
    if (memo[row][col] == 0) {
        memo[row][col] = pascal(row - 1, col - 1, memo) + pascal(row - 1, col, memo);
    }
    return memo[row][col];
}

// 打印一行杨辉三角
void print_row(int row, int memo[][10]) {
    int col = 0;
    while (col <= row) {
        putint(pascal(row, col, memo));
        if (col != row) {
            putch(32); // 空格分隔
        }
        col = col + 1;
    }
    putch(10); // 换行
}

int main() {
    // 测试shadow变量
    int n = getint();
    {
        int n = 5; // shadow外部n
        putint(n);  // 应输出5
        putch(10);
    }
    
    // 测试短路特性
    if (n > 0 && n <= 10) {
        int memo[10][10]; // 存储计算结果
        int i = 0;
        while (i < n) {
            int j = 0;
            while (j <= i) {
                memo[i][j] = 0; // 初始化
                j = j + 1;
            }
            i = i + 1;
        }
        
        // 计算并打印杨辉三角
        i = 0;
        while (i < n) {
            print_row(i, memo);
            i = i + 1;
        }
    } else {
        putch(33); // 输出'!'表示无效输入
    }
    
    return 0;
}