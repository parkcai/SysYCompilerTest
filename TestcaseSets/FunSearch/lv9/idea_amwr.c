/*
 * 测试数组初始化、逻辑运算和函数递归
 * 功能：实现汉诺塔问题的求解并输出移动步骤
 */

// 汉诺塔移动函数
void hanoi(int n, int from, int to, int via) {
    if (n == 1) {
        putch(102 + from); // 'f'
        putch(114 + from); // 'r'
        putch(111 + from); // 'o'
        putch(109 + from); // 'm'
        putch(32); // 空格
        putint(from);
        putch(32); // 空格
        putch(116 + to); // 't'
        putch(111 + to); // 'o'
        putch(32); // 空格
        putint(to);
        putch(10); // 换行
        return;
    }
    hanoi(n - 1, from, via, to);
    hanoi(1, from, to, via);
    hanoi(n - 1, via, to, from);
}

int main() {
    int n = getint(); // 获取盘子数量
    
    // 测试逻辑运算和短路求值
    if (n <= 0 || n > 10) {
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
        int n = 2;
        putint(n); // 应输出2
        putch(10); // 换行
    }
    
    // 测试数组初始化
    int towers[3][10] = {{1, 0}, {0}, {0}}; // 初始化第一个塔
    
    hanoi(n, 0, 2, 1); // 从塔0移动到塔2，借助塔1
    
    return 0;
}