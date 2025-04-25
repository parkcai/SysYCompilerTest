/*
 * 测试数组操作、递归、短路求值和变量shadow
 * 功能：实现汉诺塔问题求解并可视化移动过程
 */

// 打印汉诺塔当前状态
void print_towers(int towers[][10], int heights[]) {
    int level = 10;  // 最大高度
    while (level > 0) {
        level = level - 1;
        int tower = 0;
        while (tower < 3) {
            if (level < heights[tower]) {
                putint(towers[tower][level]);
            } else {
                putch(32); // 空格
            }
            putch(32); // 空格
            tower = tower + 1;
        }
        putch(10); // 换行
    }
    putch(45); // '-'
    putch(45); // '-'
    putch(45); // '-'
    putch(10);
}

// 移动盘子并打印状态
void move_disk(int towers[][10], int heights[], int from, int to) {
    // 更新塔状态
    heights[to] = heights[to] + 1;
    towers[to][heights[to]-1] = towers[from][heights[from]-1];
    heights[from] = heights[from] - 1;
    
    // 打印移动后的状态
    print_towers(towers, heights);
}

// 递归解决汉诺塔问题
void hanoi(int n, int towers[][10], int heights[], int from, int to, int aux) {
    if (n == 1) {
        move_disk(towers, heights, from, to);
        return;
    }
    hanoi(n - 1, towers, heights, from, aux, to);
    move_disk(towers, heights, from, to);
    hanoi(n - 1, towers, heights, aux, to, from);
}

int main() {
    // 初始化三座塔
    int towers[3][10] = {0};
    int heights[3] = {0, 0, 0};
    
    // 获取盘子数量
    int n = getint();
    if (n <= 0 || n > 10) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        return 0;
    }
    
    // 初始化第一个塔
    int i = 0;
    while (i < n) {
        towers[0][i] = n - i;
        i = i + 1;
    }
    heights[0] = n;
    
    // 测试shadow变量
    {
        int n = 999;
        putint(n); // 应输出999
        putch(10);
    }
    
    // 打印初始状态
    print_towers(towers, heights);
    
    // 解决汉诺塔问题
    hanoi(n, towers, heights, 0, 2, 1);
    
    return 0;
}