/*
 * 测试数组初始化、函数递归、逻辑运算和变量shadow
 * 功能：计算并输出汉诺塔移动步骤
 */

// 递归解决汉诺塔问题
void hanoi(int n, int from, int to, int via) {
    if (n == 1) {
        putint(from);
        putch(45); // '-'
        putch(62); // '>'
        putint(to);
        putch(10); // 换行
        return;
    }
    hanoi(n - 1, from, via, to);
    hanoi(1, from, to, via);
    hanoi(n - 1, via, to, from);
}

// 测试数组初始化和访问
void test_array() {
    int arr[3][2] = {{1, 2}, {3, 4}, {5, 6}};
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 2) {
            putint(arr[i][j]);
            putch(32); // 空格
            j = j + 1;
        }
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    int n = getint(); // 获取盘子数量
    
    // 测试短路求值
    if (n <= 0 || n > 5) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        return 0;
    }
    
    // 测试shadow变量
    {
        int n = 999;
        putint(n); // 应输出999
        putch(10); // 换行
    }
    
    test_array();
    hanoi(n, 1, 3, 2); // 从柱子1移动到柱子3，借助柱子2
    
    return 0;
}