/*
 * 测试数组操作、函数递归和逻辑运算
 * 功能：实现汉诺塔问题并验证移动步骤
 */

// 打印移动步骤
void move_disk(int disk, int from, int to) {
    putch(77); // 'M'
    putch(111); // 'o'
    putch(118); // 'v'
    putch(101); // 'e'
    putch(32); // 空格
    putint(disk);
    putch(32); // 空格
    putch(102); // 'f'
    putch(114); // 'r'
    putch(111); // 'o'
    putch(109); // 'm'
    putch(32); // 空格
    putint(from);
    putch(32); // 空格
    putch(116); // 't'
    putch(111); // 'o'
    putch(32); // 空格
    putint(to);
    putch(10); // 换行
}

// 汉诺塔递归解法
void hanoi(int n, int from, int to, int aux) {
    if (n == 1) {
        move_disk(n, from, to);
        return;
    }
    
    hanoi(n - 1, from, aux, to);
    move_disk(n, from, to);
    hanoi(n - 1, aux, to, from);
}

int main() {
    const int disks = 3;
    
    // 测试短路求值
    if (disks > 0 && disks < 4) {
        putch(84); // 'T'
        putch(10); // 换行
    }
    
    // 解决汉诺塔问题
    hanoi(disks, 1, 3, 2);
    
    // 测试shadow
    {
        int disks = 2;
        hanoi(disks, 1, 3, 2);
    }
    
    return 0;
}