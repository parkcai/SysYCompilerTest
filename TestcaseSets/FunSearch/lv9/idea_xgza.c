/*
 * 测试多维数组、函数递归、逻辑运算和变量shadow
 * 功能：计算并输出杨辉三角的前n行
 */

// 计算杨辉三角的值
void pascal_triangle(int n, int triangle[][10]) {
    int i = 0;
    while (i < n) {
        int j = 0;
        while (j <= i) {
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

// 输出杨辉三角
void print_triangle(int n, int triangle[][10]) {
    int i = 0;
    while (i < n) {
        // 测试shadow变量
        {
            int i = 0;
            while (i < n) {
                putch(42); // '*'
                i = i + 1;
            }
            putch(10);
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
    int n = getint(); // 获取行数
    
    // 测试短路求值
    if (n <= 0 || n > 10) {
        putch(69); // 'E'
        putch(114); // 'r'
        putch(114); // 'r'
        putch(111); // 'o'
        putch(114); // 'r'
        return 0;
    }
    
    int triangle[10][10];
    pascal_triangle(n, triangle);
    print_triangle(n, triangle);
    
    return 0;
}