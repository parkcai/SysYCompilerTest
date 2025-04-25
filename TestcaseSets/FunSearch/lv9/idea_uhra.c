/*
 * 测试复杂表达式计算、数组操作和函数递归
 * 功能：计算并输出汉诺塔问题的移动步骤
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

// 计算阶乘用于验证
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

int main() {
    int n = getint(); // 获取盘子数量
    
    // 测试复杂表达式和短路求值
    if (n <= 0 || (n > 10 && factorial(10) % 2 == 0)) {
        putch(69); // 'E'
        putch(114); // 'r'
        putch(114); // 'r'
        putch(111); // 'o'
        putch(114); // 'r'
        return 0;
    }
    
    // 测试数组初始化和访问
    int towers[3][10];
    int i = 0;
    while (i < n) {
        towers[0][i] = n - i;
        towers[1][i] = 0;
        towers[2][i] = 0;
        i = i + 1;
    }
    
    // 测试shadow变量
    {
        int n = 999;
        putint(n);
        putch(10);
    }
    
    // 输出汉诺塔解决方案
    hanoi(n, 1, 3, 2);
    
    // 验证阶乘计算
    putint(factorial(n));
    return 0;
}