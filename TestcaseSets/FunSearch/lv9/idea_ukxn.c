/*
 * 测试多维数组、函数递归、逻辑运算短路和变量shadow
 * 功能：实现螺旋矩阵生成并输出
 */

// 递归填充螺旋矩阵
void fill_spiral(int mat[][4], int size, int start, int offset) {
    if (size <= 0) return;
    
    // 填充上边
    int i = offset;
    while (i < offset + size) {
        mat[offset][i] = start;
        start = start + 1;
        i = i + 1;
    }
    
    // 填充右边
    i = offset + 1;
    while (i < offset + size) {
        mat[i][offset+size-1] = start;
        start = start + 1;
        i = i + 1;
    }
    
    // 填充下边
    if (size > 1) {
        i = offset + size - 2;
        while (i >= offset) {
            mat[offset+size-1][i] = start;
            start = start + 1;
            i = i - 1;
        }
    }
    
    // 填充左边
    if (size > 1) {
        i = offset + size - 2;
        while (i > offset) {
            mat[i][offset] = start;
            start = start + 1;
            i = i - 1;
        }
    }
    
    // 递归填充内层
    fill_spiral(mat, size-2, start, offset+1);
}

// 打印矩阵
void print_matrix(int mat[][4], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            putint(mat[i][j]);
            if (j < size - 1 || (j == size - 1 && i < size - 1)) {
                putch(32); // 空格
            }
            j = j + 1;
        }
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    const int size = 4;
    int mat[4][4];
    
    // 测试shadow变量
    {
        int size = getint();
        if (size > 0 && size < 5) { // 测试逻辑短路
            putch(83); // 'S'
            putch(104); // 'h'
            putch(97); // 'a'
            putch(100); // 'd'
            putch(111); // 'o'
            putch(119); // 'w'
            putch(58); // ':'
            putch(32); // 空格
            putint(size);
            putch(10); // 换行
        }
    }
    
    // 生成螺旋矩阵
    fill_spiral(mat, size, 1, 0);
    
    // 输出矩阵
    print_matrix(mat, size);
    
    return 0;
}