/*
 * 测试多维数组、递归函数、短路求值和变量shadow
 * 功能：计算并输出杨辉三角的前n行
 */

// 计算杨辉三角的值
int pascal(int row, int col) {
    // 利用递归和短路求值
    if (col == 0 || col == row || row == 0) {
        return 1;
    }
    return pascal(row - 1, col - 1) + pascal(row - 1, col);
}

// 打印杨辉三角
void print_pascal(int n) {
    int triangle[10][10];
    int i = 0;
    while (i < n) {
        int j = 0;
        while (j <= i) {
            triangle[i][j] = pascal(i, j);
            putint(triangle[i][j]);
            putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        
        // 测试变量shadow
        {
            int i = 999;
            putint(i); // 应输出999
            putch(10);
        }
        i = i + 1;
    }
}

int main() {
    int n = getint();
    
    // 测试输入合法性
    if (n <= 0 || n > 10) {
        putint(-1); // 非法输入
        return 0;
    }
    
    print_pascal(n);
    
    // 测试逻辑短路
    if (n > 5 && getint() != 0) {
        putint(1);
    } else {
        putint(0);
    }
    
    return 0;
}