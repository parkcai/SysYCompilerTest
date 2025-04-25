/*
 * 测试数组操作、递归、短路求值和变量shadow
 * 功能：实现汉诺塔问题的求解并输出移动步骤
 */

// 递归解决汉诺塔问题
void hanoi(int n, int from, int to, int temp) {
    if (n == 1) {
        putint(from);
        putch(45); // '-'
        putch(62); // '>'
        putint(to);
        putch(10); // 换行
        return;
    }
    
    // 利用短路特性防止n<=0的情况
    if (n > 1) {
        hanoi(n - 1, from, temp, to);
        hanoi(1, from, to, temp);
        hanoi(n - 1, temp, to, from);
    }
}

// 计算2的n次方
int power_of_two(int n) {
    if (n == 0) return 1;
    return 2 * power_of_two(n - 1);
}

int main() {
    int n = getint(); // 获取盘子数量
    
    // 测试短路求值
    if (n <= 0 || n > 7) {
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
        int n = 100;
        putint(n); // 应输出100
        putch(10);
    }
    
    // 输出最少步数
    putint(power_of_two(n) - 1);
    putch(10);
    
    // 输出移动步骤
    hanoi(n, 1, 3, 2);
    
    return 0;
}