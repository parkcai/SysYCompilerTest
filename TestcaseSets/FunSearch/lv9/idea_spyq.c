/*
 * 测试多维数组、递归函数、逻辑运算短路和变量shadow
 * 功能：实现汉诺塔问题求解并输出移动步骤
 */

// 汉诺塔移动函数
void hanoi(int n, int from, int to, int aux) {
    if (n == 1) {
        putint(from);
        putch(45); // '-'
        putch(62); // '>'
        putint(to);
        putch(10); // 换行
        return;
    }
    
    // 利用短路特性防止无效递归
    if (n > 1 && n < 10) {
        hanoi(n - 1, from, aux, to);
        hanoi(1, from, to, aux);
        hanoi(n - 1, aux, to, from);
    }
}

// 测试数组初始化
void init_towers(int towers[][10], int n) {
    int i = 0;
    while (i < n) {
        towers[0][i] = n - i;
        towers[1][i] = 0;
        towers[2][i] = 0;
        i = i + 1;
    }
}

int main() {
    int n = getint();
    int towers[3][10]; // 3根柱子，每根最多10个盘子
    
    // 测试shadow变量
    {
        int n = 3;
        putint(n); // 应输出3
        putch(10); // 换行
    }
    
    // 初始化汉诺塔
    init_towers(towers, n);
    
    // 解决汉诺塔问题
    hanoi(n, 0, 2, 1); // 从柱子0移动到柱子2，借助柱子1
    
    // 测试逻辑运算短路
    if (n > 0 || getint() != 0) {
        putch(79); // 'O'
        putch(75); // 'K'
        putch(10); // 换行
    }
    
    return 0;
}