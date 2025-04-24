/*
 * 测试多维数组初始化、变量shadow、逻辑运算短路和递归函数
 * 功能：实现一个简单的迷宫求解程序
 */

// 迷宫尺寸
const int N = 5;
const int M = 5;

// 方向数组
const int dirs[4][2] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};

// 递归求解迷宫路径
int solve_maze(int maze[][5], int visited[][5], int x, int y, int end_x, int end_y) {
    // 到达终点
    if (x == end_x && y == end_y) {
        return 1;
    }
    
    // 标记当前位置为已访问
    visited[x][y] = 1;
    
    // 尝试四个方向
    int i = 0;
    while (i < 4) {
        int nx = x + dirs[i][0];
        int ny = y + dirs[i][1];
        
        // 利用短路特性防止数组越界
        if (nx >= 0 && nx < N && ny >= 0 && ny < M 
            && maze[nx][ny] == 0 && visited[nx][ny] == 0) {
            if (solve_maze(maze, visited, nx, ny, end_x, end_y)) {
                return 1;
            }
        }
        i = i + 1;
    }
    
    return 0;
}

// 打印迷宫
void print_maze(int maze[][5]) {
    int i = 0;
    while (i < N) {
        int j = 0;
        while (j < M) {
            if (maze[i][j] == 1) putch(35); // '#'
            else putch(46); // '.'
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    // 迷宫定义：1表示墙，0表示路
    int maze[5][5] = {
        {0, 1, 0, 0, 0},
        {0, 1, 1, 1, 0},
        {0, 0, 0, 0, 0},
        {0, 1, 1, 1, 1},
        {0, 0, 0, 0, 0}
    };
    
    // 访问标记数组
    int visited[5][5] = {0};
    
    // 输出初始迷宫
    print_maze(maze);
    
    // 测试shadow变量
    {
        int N = 0;
        int M = 0;
        putint(N + M); // 应输出0
        putch(10);
    }
    
    // 求解迷宫路径
    if (solve_maze(maze, visited, 0, 0, 4, 4)) {
        putch(89); // 'Y'
        putch(101); // 'e'
        putch(115); // 's'
    } else {
        putch(78); // 'N'
        putch(111); // 'o'
    }
    
    return 0;
}