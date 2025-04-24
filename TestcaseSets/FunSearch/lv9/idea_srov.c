/*
 * 测试函数递归调用、数组操作和逻辑运算短路特性
 * 功能：计算汉诺塔问题的移动步骤
 */

// 递归解决汉诺塔问题
void hanoi(int n, int from, int to, int via) {
    if (n == 1) {
        putint(from);
        putch(45); // '-'
        putint(to);
        putch(32); // 空格
        return;
    }
    // 利用短路特性避免无效递归
    if (n > 0 && (from != to)) {
        hanoi(n - 1, from, via, to);
        putint(from);
        putch(45); // '-'
        putint(to);
        putch(32); // 空格
        hanoi(n - 1, via, to, from);
    }
}

int main() {
    int n = getint();
    
    // 测试数组初始化和shadow
    const int TOWER[3] = {1, 2, 3};
    {
        int TOWER[3] = {3, 2, 1}; // shadow全局TOWER数组
        putarray(3, TOWER); // 应输出3 2 1
    }
    
    // 测试逻辑运算短路
    if (n > 0 || getch() != -1) {
        hanoi(n, TOWER[0], TOWER[2], TOWER[1]);
    }
    
    return 0;
}