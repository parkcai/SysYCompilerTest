/*
 * 测试数组初始化、函数递归、逻辑运算和变量shadow
 * 功能：计算并输出杨辉三角的前n行
 */

// 计算杨辉三角的值
int pascal(int row, int col) {
    if (col == 0 || col == row) {
        return 1;
    }
    // 测试逻辑短路
    if (row < 0 || col < 0 || col > row) {
        return 0;
    }
    return pascal(row - 1, col - 1) + pascal(row - 1, col);
}

// 打印杨辉三角
void print_pascal(int n) {
    int i = 0;
    while (i < n) {
        int j = 0;
        while (j <= i) {
            putint(pascal(i, j));
            putch(32);  // 空格
            j = j + 1;
        }
        putch(10);  // 换行
        i = i + 1;
    }
}

int main() {
    int n = getint();
    
    // 测试shadow变量
    {
        int n = 5;
        putint(n);  // 应输出5而非输入值
        putch(10);
    }
    
    // 测试输入合法性
    if (n <= 0) {
        putch(33);  // 输出'!'表示错误
        return -1;
    }
    
    // 打印杨辉三角
    print_pascal(n);
    
    return 0;
}