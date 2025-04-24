/*
 * 测试多维数组、递归、变量shadow和逻辑运算短路
 * 功能：实现一个简单的井字棋游戏判断
 */

// 检查行是否有玩家获胜
int check_row(int board[][3], int player) {
    int i = 0;
    while (i < 3) {
        if (board[i][0] == player && board[i][1] == player && board[i][2] == player) {
            return 1;
        }
        i = i + 1;
    }
    return 0;
}

// 检查列是否有玩家获胜
int check_col(int board[][3], int player) {
    int j = 0;
    while (j < 3) {
        if (board[0][j] == player && board[1][j] == player && board[2][j] == player) {
            return 1;
        }
        j = j + 1;
    }
    return 0;
}

// 检查对角线是否有玩家获胜
int check_diag(int board[][3], int player) {
    // 主对角线
    if (board[0][0] == player && board[1][1] == player && board[2][2] == player) {
        return 1;
    }
    // 副对角线
    if (board[0][2] == player && board[1][1] == player && board[2][0] == player) {
        return 1;
    }
    return 0;
}

// 判断玩家是否获胜
int is_winner(int board[][3], int player) {
    return check_row(board, player) || check_col(board, player) || check_diag(board, player);
}

// 打印棋盘
void print_board(int board[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            if (board[i][j] == 1) putch(88); // 'X'
            else if (board[i][j] == 2) putch(79); // 'O'
            else putch(95); // '_'
            putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    // 初始化棋盘 (0:空, 1:X, 2:O)
    int board[3][3] = {
        {1, 0, 2},
        {0, 1, 0},
        {2, 0, 1}
    };
    
    // 打印棋盘
    print_board(board);
    
    // 测试变量shadow
    {
        int board = 123;
        putint(board); // 应输出123
        putch(10);
    }
    
    // 检查X玩家是否获胜
    if (is_winner(board, 1)) {
        putch(88); // 'X'
        putch(32);
        putch(119); // 'w'
        putch(105); // 'i'
        putch(110); // 'n'
        putch(115); // 's'
    } else if (is_winner(board, 2)) { // 检查O玩家是否获胜
        putch(79); // 'O'
        putch(32);
        putch(119); // 'w'
        putch(105); // 'i'
        putch(110); // 'n'
        putch(115); // 's'
    } else {
        putch(78); // 'N'
        putch(111); // 'o'
        putch(32); // ' '
        putch(119); // 'w'
        putch(105); // 'i'
        putch(110); // 'n'
        putch(110); // 'n'
        putch(101); // 'e'
        putch(114); // 'r'
    }
    
    return 0;
}