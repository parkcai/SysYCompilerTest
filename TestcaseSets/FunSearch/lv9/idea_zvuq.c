/*
 * 测试常量数组、变量shadow、短路求值和多维数组访问
 * 功能：实现简单的井字棋游戏并验证胜利条件
 */

// 游戏板状态常量
const int EMPTY = 0;
const int PLAYER_X = 1;
const int PLAYER_O = 2;

// 检查行是否获胜
int check_row(int board[][3], int row, int player) {
    return board[row][0] == player && 
           board[row][1] == player && 
           board[row][2] == player;
}

// 检查列是否获胜
int check_col(int board[][3], int col, int player) {
    return board[0][col] == player && 
           board[1][col] == player && 
           board[2][col] == player;
}

// 检查对角线是否获胜
int check_diag(int board[][3], int player) {
    return (board[0][0] == player && 
            board[1][1] == player && 
            board[2][2] == player) ||
           (board[0][2] == player && 
            board[1][1] == player && 
            board[2][0] == player);
}

// 检查玩家是否获胜
int check_win(int board[][3], int player) {
    int i = 0;
    while (i < 3) {
        if (check_row(board, i, player) || check_col(board, i, player)) {
            return 1;
        }
        i = i + 1;
    }
    return check_diag(board, player);
}

// 打印游戏板
void print_board(int board[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            if (board[i][j] == EMPTY) {
                putch(45); // '-'
            } else if (board[i][j] == PLAYER_X) {
                putch(88); // 'X'
            } else {
                putch(79); // 'O'
            }
            putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    // 初始化游戏板
    int board[3][3] = {
        {EMPTY, EMPTY, EMPTY},
        {EMPTY, EMPTY, EMPTY},
        {EMPTY, EMPTY, EMPTY}
    };
    
    // 测试游戏逻辑
    board[0][0] = PLAYER_X;
    board[1][1] = PLAYER_X;
    board[2][2] = PLAYER_X;
    
    // 测试shadow变量
    {
        int board = 123;
        putint(board); // 应输出123
        putch(10);
    }
    
    // 打印游戏板
    print_board(board);
    
    // 检查胜利条件（应输出1）
    putint(check_win(board, PLAYER_X));
    
    return 0;
}