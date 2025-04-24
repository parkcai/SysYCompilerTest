/*
 * 测试多维数组、函数递归和逻辑运算
 * 功能：计算并输出帕斯卡三角形的前n行
 */

// 计算帕斯卡三角形值
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
        // 打印空格
        int space = 0;
        while (space < n - i - 1) {
            putch(32); // 空格
            space = space + 1;
        }
        
        // 打印数字
        int j = 0;
        while (j <= i) {
            putint(pascal(i, j));
            if (j < i) {
                putch(32); // 数字间空格
            }
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    // 测试shadow变量
    int n = getint();
    {
        int n = 5;
        putint(n); // 应输出5
        putch(10);
    }
    
    // 测试短路特性
    if (n > 0 && (100 / n > 0)) {
        print_pascal(n);
    } else {
        putch(33); // 输出'!'表示无效输入
    }
    
    return 0;
}