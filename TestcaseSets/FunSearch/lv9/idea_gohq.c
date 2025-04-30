/*
 * 测试数组操作、函数递归、逻辑运算和变量shadow
 * 功能：实现汉诺塔问题的求解并输出移动步骤
 */

// 汉诺塔移动步骤记录函数
void hanoi(int n, int from, int to, int via) {
    if (n == 1) {
        putch(77); // 'M'
        putch(111); // 'o'
        putch(118); // 'v'
        putch(101); // 'e'
        putch(32); // 空格
        putint(from);
        putch(32); // 空格
        putch(116); // 't'
        putch(111); // 'o'
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
    int n = getint();
    
    // 测试shadow变量
    {
        int n = 2; // shadow变量
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
        hanoi(n, 1, 3, 2); // 输出shadow变量对应的汉诺塔步骤
    }
    
    // 输出实际输入值对应的汉诺塔步骤
    if (n > 0 && n < 5) { // 测试逻辑短路
        hanoi(n, 1, 3, 2);
    } else {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        putch(32); // 空格
        putch(110); // 'n'
        putch(10); // 换行
    }
    
    return 0;
}