/*
 * 测试数组操作、递归函数、短路求值和变量shadow
 * 功能：实现汉诺塔问题的求解并输出移动步骤
 */

// 汉诺塔递归求解
void hanoi(int n, int from, int to, int aux) {
    if (n == 1) {
        putint(from);
        putch(45); // '-'
        putch(62); // '>'
        putint(to);
        putch(10); // 换行
        return;
    }
    
    // 递归移动n-1个盘子
    hanoi(n - 1, from, aux, to);
    
    // 移动最下面的盘子
    putint(from);
    putch(45); // '-'
    putch(62); // '>'
    putint(to);
    putch(10); // 换行
    
    // 递归移动n-1个盘子
    hanoi(n - 1, aux, to, from);
}

int main() {
    // 测试shadow变量
    int n = getint();
    {
        int n = 3; // shadow外部n
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
    
    // 测试短路求值
    if (n > 0 && n < 10) {
        // 柱子编号1,2,3
        hanoi(n, 1, 3, 2);
    } else {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        putch(33); // '!'
    }
    
    return 0;
}