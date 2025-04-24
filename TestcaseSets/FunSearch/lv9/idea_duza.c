/*
 * 测试多维数组初始化、函数递归调用和逻辑运算
 * 功能：计算并输出帕斯卡三角形
 */

// 计算帕斯卡三角形值
int pascal_value(int row, int col) {
    if (col == 0 || col == row) {
        return 1;
    }
    return pascal_value(row - 1, col - 1) + pascal_value(row - 1, col);
}

// 打印帕斯卡三角形
void print_pascal(int n) {
    int i = 0;
    while (i < n) {
        int j = 0;
        while (j <= i) {
            putint(pascal_value(i, j));
            putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

// 测试数组初始化和shadow
void test_shadow() {
    int arr[3] = {1, 2, 3};
    putarray(3, arr); // 输出1 2 3
    
    {
        int arr[3] = {4, 5, 6}; // shadow外部arr
        putarray(3, arr); // 输出4 5 6
    }
    
    putarray(3, arr); // 再次输出1 2 3
}

int main() {
    int n = getint();
    
    // 测试逻辑运算短路
    if (n > 0 && n < 10) {
        print_pascal(n);
    } else {
        putch(33); // 输出'!'表示输入不合法
    }
    
    // 测试shadow功能
    test_shadow();
    
    return 0;
}