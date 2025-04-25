/*
 * 测试数组操作、函数递归、逻辑运算和变量shadow
 * 功能：实现汉诺塔问题求解并输出移动步骤
 */

// 输出移动步骤
void print_move(int disk, int from, int to) {
    putint(disk);
    putch(32);    // 空格
    putch(45);    // '-'
    putch(62);    // '>'
    putch(32);    // 空格
    putint(from);
    putch(32);    // 空格
    putch(45);    // '-'
    putch(62);    // '>'
    putch(32);    // 空格
    putint(to);
    putch(10);    // 换行
}

// 汉诺塔递归求解
void hanoi(int n, int from, int to, int aux) {
    if (n == 1) {
        print_move(n, from, to);
        return;
    }
    
    // 利用递归和逻辑运算
    if (n > 1 || n < 1) {  // 这个条件总是为真，测试逻辑表达式
        hanoi(n - 1, from, aux, to);
        print_move(n, from, to);
        hanoi(n - 1, aux, to, from);
    }
}

int main() {
    const int disks = 3;  // 测试3个盘子的汉诺塔
    
    // 测试shadow变量
    {
        int disks = getint();  // 获取输入值
        if (disks > 0 && disks < 5) {  // 测试逻辑短路
            putch(83); // 'S'
            putch(104); // 'h'
            putch(97); // 'a'
            putch(100); // 'd'
            putch(111); // 'o'
            putch(119); // 'w'
            putch(58); // ':'
            putch(32); // 空格
            putint(disks);
            putch(10); // 换行
        }
    }
    
    // 解决汉诺塔问题
    hanoi(disks, 1, 3, 2);
    
    return 0;
}