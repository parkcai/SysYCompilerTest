/*
 * 测试多维数组初始化、函数递归、逻辑运算和变量shadow
 * 功能：实现汉诺塔问题求解并输出移动步骤
 */

// 汉诺塔递归求解
void hanoi(int n, int from, int to, int via) {
    if (n == 1) {
        putint(from);
        putch(45); // '-'
        putch(62); // '>'
        putint(to);
        putch(10);
        return;
    }
    
    // 测试短路求值
    if (n > 0 && n < 10) {
        hanoi(n - 1, from, via, to);
        hanoi(1, from, to, via);
        hanoi(n - 1, via, to, from);
    }
}

int main() {
    // 测试多维数组初始化
    int towers[3][5] = {{1, 0, 0, 0, 0}, {2, 3, 4, 5, 0}, {0, 0, 0, 0, 0}};
    
    // 测试shadow变量
    {
        int n = 3;
        putint(n); // 应输出3
        putch(10);
    }
    
    int n = getint(); // 获取盘子数量
    
    // 测试逻辑运算
    if (n <= 0 || n > 5) {
        putint(-1); // 输入不合法
        return 0;
    }
    
    // 初始化塔
    int i = 0;
    while (i < n) {
        towers[0][i] = i + 1;
        i = i + 1;
    }
    
    // 解决汉诺塔问题
    hanoi(n, 1, 3, 2);
    
    return 0;
}