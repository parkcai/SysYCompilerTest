/*
 * 测试数组操作、函数递归、逻辑运算和变量shadow
 * 功能：实现汉诺塔问题并输出移动步骤
 */

// 输出移动步骤
void move_disk(int n, int from, int to) {
    putint(n);
    putch(32);  // 空格
    putint(from);
    putch(32);
    putint(to);
    putch(10);  // 换行
}

// 汉诺塔递归解法
void hanoi(int n, int from, int to, int aux) {
    if (n == 1) {
        move_disk(n, from, to);
        return;
    }
    // 利用短路特性避免无效递归
    if (n > 1 && n < 10) {
        hanoi(n - 1, from, aux, to);
        move_disk(n, from, to);
        hanoi(n - 1, aux, to, from);
    }
}

int main() {
    int n = getint();  // 获取盘子数量
    
    // 测试shadow
    {
        int n = 3;  // shadow外部变量
        putint(n);   // 输出3
        putch(10);   // 换行
    }
    
    // 测试逻辑运算
    if (n <= 0 || n > 8) {
        putint(-1);  // 非法输入
    } else {
        hanoi(n, 1, 3, 2);  // 从柱子1移动到柱子3，柱子2作为辅助
    }
    
    return 0;
}