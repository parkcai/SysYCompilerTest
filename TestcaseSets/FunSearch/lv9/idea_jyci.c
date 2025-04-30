/*
 * 测试多维数组初始化、函数递归调用、短路逻辑运算和变量shadow
 * 功能：计算并输出帕斯卡三角形
 */

// 计算帕斯卡三角形元素
int pascal(int row, int col) {
    if (col == 0 || col == row) {
        return 1;
    } else {
        return pascal(row - 1, col - 1) + pascal(row - 1, col);
    }
}

// 打印帕斯卡三角形
void print_pascal(int n) {
    int i = 0;
    while (i < n) {
        // 打印前导空格
        int space = 0;
        while (space < n - i - 1) {
            putch(32); // 空格
            space = space + 1;
        }
        
        // 打印数字
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
    // 测试变量shadow
    int n = getint();
    {
        int n = 5;
        putint(n); // 应输出5而非输入值
        putch(10);
    }
    
    // 测试短路逻辑运算
    if (n <= 0 || n > 10) {
        putch(69); // 'E'
        putch(114); // 'r'
        putch(114); // 'r'
        putch(111); // 'o'
        putch(114); // 'r'
        putch(10);
        return 0;
    }
    
    // 测试多维数组初始化
    int triangle[10][10];
    int i = 0;
    while (i < n) {
        int j = 0;
        while (j <= i) {
            triangle[i][j] = pascal(i, j);
            j = j + 1;
        }
        i = i + 1;
    }
    
    print_pascal(n);
    return 0;
}