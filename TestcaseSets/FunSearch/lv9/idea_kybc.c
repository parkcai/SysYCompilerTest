/*
 * 测试递归函数、数组操作、短路求值和变量shadow
 * 功能：实现汉诺塔问题求解并输出移动步骤
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
    
    // 测试短路求值
    if (n > 0 && from != to) {
        hanoi(n - 1, from, via, to);
        hanoi(1, from, to, via);
        hanoi(n - 1, via, to, from);
    }
}

// 计算2的n次方
int power_of_two(int n) {
    if (n == 0) return 1;
    return 2 * power_of_two(n - 1);
}

int main() {
    const int MAX_DISKS = 5;
    int n = getint();
    
    // 检查输入合法性
    if (n <= 0 || n > MAX_DISKS) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        putch(33); // '!'
        return 0;
    }
    
    // 测试shadow变量
    {
        int n = 3;
        putch(83); // 'S'
        putch(104); // 'h'
        putch(97); // 'a'
        putch(100); // 'd'
        putch(111); // 'o'
        putch(119); // 'w'
        putch(58); // ':'
        putch(32); // 空格
        putint(n);
        putch(10); // 换行
    }
    
    // 输出最少移动次数
    putch(77); // 'M'
    putch(111); // 'o'
    putch(118); // 'v'
    putch(101); // 'e'
    putch(115); // 's'
    putch(58); // ':'
    putch(32); // 空格
    putint(power_of_two(n) - 1);
    putch(10); // 换行
    
    // 输出移动步骤
    hanoi(n, 1, 3, 2);
    
    return 0;
}