/*
 * 测试数组初始化、函数递归、逻辑运算和变量shadow
 * 功能：计算并输出汉诺塔问题的解决步骤
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

// 打印欢迎信息
void print_welcome() {
    putch(72); // 'H'
    putch(97); // 'a'
    putch(110); // 'n'
    putch(111); // 'o'
    putch(105); // 'i'
    putch(32); // ' '
    putch(84); // 'T'
    putch(111); // 'o'
    putch(119); // 'w'
    putch(101); // 'e'
    putch(114); // 'r'
    putch(10); // 换行
}

int main() {
    // 测试shadow变量
    int n = getint();
    {
        int n = 3;
        putint(n); // 应输出3而非输入值
        putch(10);
    }
    
    // 测试短路求值
    if (n <= 0 || n > 10) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        putch(10);
        return 0;
    }
    
    print_welcome();
    hanoi(n, 1, 3, 2);
    
    return 0;
}