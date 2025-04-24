/*
 * 测试函数递归、数组操作、逻辑运算和变量shadow
 * 功能：模拟汉诺塔问题并输出移动步骤
 */

// 输出移动步骤
void print_move(int disk, int from, int to) {
    putch(68); // 'D'
    putch(105); // 'i'
    putch(115); // 's'
    putch(107); // 'k'
    putch(32); // ' '
    putint(disk);
    putch(32); // ' '
    putch(109); // 'm'
    putch(111); // 'o'
    putch(118); // 'v'
    putch(101); // 'e'
    putch(100); // 'd'
    putch(32); // ' '
    putch(102); // 'f'
    putch(114); // 'r'
    putch(111); // 'o'
    putch(109); // 'm'
    putch(32); // ' '
    putint(from);
    putch(32); // ' '
    putch(116); // 't'
    putch(111); // 'o'
    putch(32); // ' '
    putint(to);
    putch(10); // 换行
}

// 汉诺塔递归解法
void hanoi(int n, int from, int to, int aux) {
    if (n == 1) {
        print_move(n, from, to);
        return;
    }
    hanoi(n - 1, from, aux, to);
    print_move(n, from, to);
    hanoi(n - 1, aux, to, from);
}

int main() {
    int n = getint(); // 获取盘子数量
    
    // 测试短路求值
    if (n <= 0 || n > 8) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        putch(32); // ' '
        putch(105); // 'i'
        putch(110); // 'n'
        putch(112); // 'p'
        putch(117); // 'u'
        putch(116); // 't'
        return 0;
    }
    
    // 测试shadow变量
    {
        int n = 2;
        putint(n); // 应输出2
        putch(10); // 换行
    }
    
    hanoi(n, 1, 3, 2); // 从柱子1移动到柱子3，借助柱子2
    
    return 0;
}