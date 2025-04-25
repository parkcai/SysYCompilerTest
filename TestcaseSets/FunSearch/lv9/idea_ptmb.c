/*
 * 测试数组参数传递、多维数组初始化、逻辑运算短路和递归
 * 功能：计算并输出N阶魔方阵
 */

// 初始化魔方阵
void init_magic_square(int n, int square[][10]) {
    int row = 0;
    int col = n / 2;
    int num = 1;
    
    while (num <= n * n) {
        square[row][col] = num;
        num = num + 1;
        
        int next_row = (row - 1 + n) % n;
        int next_col = (col + 1) % n;
        
        // 使用短路特性防止重复赋值
        if (square[next_row][next_col] != 0 || (row == 0 && col == n - 1)) {
            row = (row + 1) % n;
        } else {
            row = next_row;
            col = next_col;
        }
    }
}

// 递归打印二维数组
void print_square_recursive(int square[][10], int n, int row) {
    if (row >= n) {
        return;
    }
    
    int col = 0;
    while (col < n) {
        putint(square[row][col]);
        putch(32); // 空格
        col = col + 1;
    }
    putch(10); // 换行
    
    print_square_recursive(square, n, row + 1); // 递归调用
}

int main() {
    int n = getint();
    
    // 测试短路求值
    if (n <= 0 || n > 10 || (n % 2 == 0)) {
        putint(-1); // 非法输入
        return 0;
    }
    
    int magic_square[10][10] = {0}; // 初始化二维数组
    init_magic_square(n, magic_square);
    
    // 测试shadow
    {
        int n = 5;
        putint(n); // 应输出5
        putch(10);
    }
    
    print_square_recursive(magic_square, n, 0);
    return 0;
}