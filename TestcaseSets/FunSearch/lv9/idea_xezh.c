/*
 * 测试数组操作、函数递归和逻辑运算
 * 功能：模拟汉诺塔问题并输出移动步骤
 */

// 打印移动步骤
void print_move(int disk, int from, int to) {
    putint(disk);
    putch(32);
    putch(102); // 'f'
    putch(114); // 'r'
    putch(111); // 'o'
    putch(109); // 'm'
    putch(32);
    putint(from);
    putch(32);
    putch(116); // 't'
    putch(111); // 'o'
    putch(32);
    putint(to);
    putch(10);
}

// 汉诺塔递归解法
void hanoi(int n, int from, int to, int aux) {
    if (n == 1) {
        print_move(n, from, to);
    } else {
        hanoi(n - 1, from, aux, to);
        print_move(n, from, to);
        hanoi(n - 1, aux, to, from);
    }
}

int main() {
    int n = getint();
    
    // 测试短路特性
    if (n > 0 && n < 10) {
        // 测试shadow变量
        {
            int n = 3;
            hanoi(n, 1, 3, 2); // 应输出3层汉诺塔解法
        }
        
        // 输出实际解法
        hanoi(n, 1, 3, 2);
    } else {
        putch(33); // 输出'!'表示无效输入
    }
    
    return 0;
}