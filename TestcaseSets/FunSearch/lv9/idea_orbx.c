/*
 * 测试多维数组、递归函数、短路逻辑和变量shadow
 * 功能：实现汉诺塔问题求解并输出移动步骤
 */

// 递归解决汉诺塔问题
void hanoi(int n, int from, int to, int aux) {
    if (n == 1) {
        putint(from);
        putch(45); // '-'
        putch(62); // '>'
        putint(to);
        putch(10); // 换行
        return;
    }
    
    // 测试短路逻辑：n > 1才会继续执行
    if (n > 1 && from != to) {
        hanoi(n - 1, from, aux, to);
        hanoi(1, from, to, aux);
        hanoi(n - 1, aux, to, from);
    }
}

int main() {
    // 测试shadow变量
    int n = getint();
    {
        int n = 3; // shadow外部n
        putint(n); // 应输出3
        putch(10);
    }
    
    // 测试多维数组初始化
    int towers[3][5] = {{1, 2, 3, 4, 5}, {0}, {0}};
    
    // 输出初始状态
    putarray(5, towers[0]);
    
    // 解决汉诺塔问题
    hanoi(n, 0, 2, 1);
    
    return 0;
}