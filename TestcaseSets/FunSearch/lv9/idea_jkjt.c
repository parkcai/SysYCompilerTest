/*
 * 测试多维数组、递归函数、逻辑运算短路和变量shadow
 * 功能：计算并输出杨辉三角
 */

// 递归计算杨辉三角的值
int pascal(int row, int col) {
    if (col == 0 || col == row) {
        return 1;
    }
    return pascal(row - 1, col - 1) + pascal(row - 1, col);
}

// 输出杨辉三角
void print_pascal(int n) {
    int i = 0;
    while (i < n) {
        // 测试shadow变量
        {
            int i = 0; // shadow外层i
            while (i <= n) {
                putch(32); // 空格
                i = i + 1;
            }
        }
        
        int j = 0;
        while (j <= i) {
            putint(pascal(i, j));
            putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    int n = getint();
    
    // 测试短路特性
    if (n <= 0 || n > 10) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        putch(32); // ' '
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(117); // 'u'
        putch(101); // 'e'
        return 0;
    }
    
    print_pascal(n);
    
    return 0;
}