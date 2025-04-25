/*
 * 测试多维数组、递归函数、逻辑运算和变量shadow
 * 功能：计算并输出杨辉三角的前n行
 */

// 计算杨辉三角的值
int yanghui(int row, int col) {
    if (col == 0 || col == row) {
        return 1;
    }
    return yanghui(row - 1, col - 1) + yanghui(row - 1, col);
}

// 打印杨辉三角
void print_yanghui(int n) {
    int i = 0;
    while (i < n) {
        int j = 0;
        // 测试变量shadow
        {
            int i = 999;
            putint(i);  // 应输出999
            putch(10);  // 换行
        }
        
        while (j <= i) {
            putint(yanghui(i, j));
            putch(32);  // 空格
            j = j + 1;
        }
        putch(10);  // 换行
        i = i + 1;
    }
}

int main() {
    int n = getint();
    int triangle[10][10];  // 二维数组存储杨辉三角
    
    // 测试二维数组初始化
    {
        int i = 0;
        while (i < 10) {
            int j = 0;
            while (j <= i) {
                triangle[i][j] = yanghui(i, j);
                j = j + 1;
            }
            i = i + 1;
        }
    }
    
    // 测试输入合法性
    if (n > 0 && n <= 10) {
        print_yanghui(n);
    } else {
        putint(-1);  // 非法输入
    }
    
    return 0;
}