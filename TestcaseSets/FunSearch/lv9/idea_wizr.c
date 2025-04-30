/*
 * 测试多维数组、递归函数、逻辑短路和变量shadow
 * 功能：实现汉诺塔问题求解并验证结果
 */

// 递归解决汉诺塔问题
void hanoi(int n, int from, int to, int aux, int moves[][3]) {
    if (n == 1) {
        moves[from][to] = moves[from][to] + 1;
        return;
    }
    hanoi(n - 1, from, aux, to, moves);
    moves[from][to] = moves[from][to] + 1;
    hanoi(n - 1, aux, to, from, moves);
}

// 初始化移动计数矩阵
void init_moves(int moves[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            moves[i][j] = 0;
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印移动计数矩阵
void print_moves(int moves[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            putint(moves[i][j]);
            if (j < 2) {
                putch(32); // 空格
            }
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    int n = getint(); // 获取盘子数量
    
    // 测试逻辑短路
    if (n > 0 && n < 10) {
        int moves[3][3];
        init_moves(moves);
        
        // 测试变量shadow
        {
            int n = 3; // shadow外层n
            hanoi(n, 0, 2, 1, moves);
            print_moves(moves);
            init_moves(moves); // 重新初始化
        }
        
        hanoi(n, 0, 2, 1, moves);
        print_moves(moves);
    } else {
        putch(33); // 输出'!'表示无效输入
    }
    
    return 0;
}