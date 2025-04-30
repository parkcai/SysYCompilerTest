/*
 * 测试多维数组、递归函数、变量shadow和逻辑运算短路
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

// 初始化3x3矩阵
void init_matrix(int mat[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            mat[i][j] = 0;
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印矩阵
void print_matrix(int mat[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            putint(mat[i][j]);
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
    // 测试汉诺塔问题
    int moves[3][3];
    init_matrix(moves);
    
    int n = getint(); // 获取盘子数量
    if (n > 0 && n < 10) { // 测试逻辑短路
        hanoi(n, 0, 2, 1, moves);
    } else {
        putch(33); // 输出'!'表示无效输入
        return 0;
    }
    
    // 输出移动矩阵
    print_matrix(moves);
    
    // 测试变量shadow
    {
        int n = 5;
        putint(n); // 输出5
        putch(10);
    }
    
    return 0;
}