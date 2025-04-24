/*
 * 测试多维数组初始化、函数递归、逻辑运算短路和变量shadow
 * 功能：计算并输出帕斯卡三角形
 */

// 计算帕斯卡三角形元素
int pascal(int row, int col) {
    if (col == 0 || col == row) {
        return 1;
    }
    // 递归计算并利用短路特性避免无效递归
    if (row > 0 && col > 0) {
        return pascal(row - 1, col - 1) + pascal(row - 1, col);
    }
    return 0;
}

// 打印帕斯卡三角形
void print_pascal(int n) {
    int i = 0;
    while (i < n) {
        // 测试shadow
        {
            int i = 0;
            while (i < n - 1) {
                putch(32); // 空格对齐
                i = i + 1;
            }
        }
        
        int j = 0;
        while (j <= i) {
            putint(pascal(i, j));
            putch(32);
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    const int MAX_ROW = 10;
    int n = getint();
    
    // 测试多维数组初始化
    int triangle[5][5] = {{1}, {1,1}, {1,2,1}, {1,3,3,1}, {1,4,6,4,1}};
    putarray(5, triangle[4]); // 输出第5行
    
    // 测试逻辑运算短路
    if (n > 0 && n <= MAX_ROW) {
        print_pascal(n);
    } else {
        putint(-1); // 输入不合法
    }
    
    return 0;
}