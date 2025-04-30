/*
 * 测试多维数组初始化、函数递归、逻辑运算和变量shadow
 * 功能：实现八皇后问题并输出所有解
 */

const int N = 8;  // 棋盘大小

// 打印棋盘
void print_board(int board[][8]) {
    int i = 0;
    while (i < N) {
        int j = 0;
        while (j < N) {
            if (board[i][j]) {
                putch(81); // 'Q'
            } else {
                putch(46); // '.'
            }
            putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
    putch(10); // 空行分隔解
}

// 检查当前位置是否安全
int is_safe(int board[][8], int row, int col) {
    int i = 0;
    int j = 0;
    
    // 检查左侧行
    while (i < col) {
        if (board[row][i]) {
            return 0;
        }
        i = i + 1;
    }
    
    // 检查左上对角线
    i = row;
    j = col;
    while (i >= 0 && j >= 0) {
        if (board[i][j]) {
            return 0;
        }
        i = i - 1;
        j = j - 1;
    }
    
    // 检查左下对角线
    i = row;
    j = col;
    while (i < N && j >= 0) {
        if (board[i][j]) {
            return 0;
        }
        i = i + 1;
        j = j - 1;
    }
    
    return 1;
}

// 递归解决八皇后问题
int solve_nq(int board[][8], int col) {
    if (col >= N) {
        print_board(board);
        return 1;
    }
    
    int res = 0;
    int i = 0;
    while (i < N) {
        if (is_safe(board, i, col)) {
            // 测试shadow变量
            {
                int board[8][8];
                board[i][col] = 1;
            }
            
            board[i][col] = 1;
            res = solve_nq(board, col + 1) || res;
            board[i][col] = 0; // 回溯
        }
        i = i + 1;
    }
    
    return res;
}

int main() {
    int board[8][8] = {{0}}; // 初始化棋盘
    
    if (!solve_nq(board, 0)) {
        putch(78); // 'N'
        putch(111); // 'o'
        putch(32); // ' '
        putch(115); // 's'
        putch(111); // 'o'
        putch(108); // 'l'
        putch(117); // 'u'
        putch(116); // 't'
        putch(105); // 'i'
        putch(111); // 'o'
        putch(110); // 'n'
    }
    
    return 0;
}