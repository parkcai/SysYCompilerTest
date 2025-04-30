/*
 * 测试数组操作、函数递归、短路逻辑和变量shadow
 * 功能：实现汉诺塔问题求解并输出移动步骤
 */

// 汉诺塔移动函数
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
    
    // 递归调用
    hanoi(n - 1, from, via, to);
    hanoi(1, from, to, via);
    hanoi(n - 1, via, to, from);
}

// 验证输入有效性
int validate_input(int n) {
    // 测试短路逻辑
    if (n <= 0 || n > 10) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        putch(32); // 空格
        putch(105); // 'i'
        putch(110); // 'n'
        putch(112); // 'p'
        putch(117); // 'u'
        putch(116); // 't'
        return 0;
    }
    return 1;
}

int main() {
    int n = getint();
    
    // 测试变量shadow
    {
        int n = 2;
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
    
    if (validate_input(n)) {
        hanoi(n, 1, 3, 2);
    }
    
    return 0;
}