/*
 * 测试多维数组、递归函数、短路逻辑和变量shadow
 * 功能：模拟井字棋游戏并判断胜负
 */

// 初始化3x3棋盘
void init_board(int board[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            board[i][j] = 0; // 0表示空位
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印棋盘
void print_board(int board[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            if (board[i][j] == 0) {
                putch(46); // '.'表示空位
            } else if (board[i][j] == 1) {
                putch(88); // 'X'表示玩家1
            } else {
                putch(79); // 'O'表示玩家2
            }
            putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

// 检查是否有玩家获胜
int check_win(int board[][3]) {
    // 检查行
    int i = 0;
    while (i < 3) {
        if (board[i][0] != 0 && board[i][0] == board[i][1] && board[i][1] == board[i][2]) {
            return board[i][0];
        }
        i = i + 1;
    }
    
    // 检查列
    int j = 0;
    while (j < 3) {
        if (board[0][j] != 0 && board[0][j] == board[1][j] && board[1][j] == board[2][j]) {
            return board[0][j];
        }
        j = j + 1;
    }
    
    // 检查对角线
    if (board[0][0] != 0 && board[0][0] == board[1][1] && board[1][1] == board[2][2]) {
        return board[0][0];
    }
    if (board[0][2] != 0 && board[0][2] == board[1][1] && board[1][1] == board[2][0]) {
        return board[0][2];
    }
    
    return 0; // 无人获胜
}

// 递归计算所有可能的走法
int count_moves(int board[][3], int player) {
    int total = 0;
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            if (board[i][j] == 0) {
                board[i][j] = player;
                int winner = check_win(board);
                if (winner == 0) {
                    total = total + count_moves(board, 3 - player); // 切换玩家
                } else {
                    total = total + 1;
                }
                board[i][j] = 0; // 回溯
            }
            j = j + 1;
        }
        i = i + 1;
    }
    return total;
}

int main() {
    int board[3][3];
    init_board(board);
    
    // 测试变量shadow
    {
        int board[2][2] = {{1, 2}, {3, 4}};
        putarray(2, board[0]); // 输出1 2
        putarray(2, board[1]); // 输出3 4
    }
    
    // 模拟游戏过程
    int moves = getint();
    while (moves > 0) {
        int player = getint();
        int x = getint();
        int y = getint();
        
        // 使用短路逻辑防止数组越界
        if (x >= 0 && x < 3 && y >= 0 && y < 3 && board[x][y] == 0) {
            board[x][y] = player;
        }
        moves = moves - 1;
    }
    
    // 打印最终棋盘
    print_board(board);
    
    // 检查获胜者
    int winner = check_win(board);
    if (winner != 0) {
        putch(87); // 'W'
        putch(105); // 'i'
        putch(110); // 'n'
        putch(110); // 'n'
        putch(101); // 'e'
        putch(114); // 'r'
        putch(58); // ':'
        putch(32); // 空格
        putint(winner);
    } else {
        putch(78); // 'N'
        putch(111); // 'o'
        putch(32); // 空格
        putch(119); // 'w'
        putch(105); // 'i'
        putch(110); // 'n'
        putch(110); // 'n'
        putch(101); // 'e'
        putch(114); // 'r'
    }
    
    // 测试递归函数
    putch(10); // 换行
    putint(count_moves(board, 1));
    
    return 0;
}