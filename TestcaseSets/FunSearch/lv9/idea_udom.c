/*
 * 测试多维数组、递归、逻辑运算和变量shadow
 * 功能：计算并输出杨辉三角
 */

// 计算杨辉三角的值
void pascal_triangle(int rows, int triangle[][100]) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j <= i) {
            // 测试逻辑运算短路
            if (j == 0 || j == i) {
                triangle[i][j] = 1;
            } else {
                triangle[i][j] = triangle[i-1][j-1] + triangle[i-1][j];
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印杨辉三角
void print_triangle(int rows, int triangle[][100]) {
    int i = 0;
    while (i < rows) {
        // 测试变量shadow
        {
            int i = -1;
            putint(i); // 应输出-1
            putch(32); // 空格
        }
        
        int j = 0;
        while (j <= i) {
            putint(triangle[i][j]);
            putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    int rows = getint();
    if (rows <= 0 || rows > 100) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        putch(32); // 空格
        putch(82); // 'R'
        putch(111); // 'o'
        putch(119); // 'w'
        putch(115); // 's'
        return 0;
    }
    
    int triangle[100][100];
    pascal_triangle(rows, triangle);
    print_triangle(rows, triangle);
    
    return 0;
}