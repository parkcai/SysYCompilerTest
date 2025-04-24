/*
 * 测试多维数组、函数递归和逻辑运算
 * 功能：计算并输出杨辉三角的前n行
 */

// 计算杨辉三角的值
void pascal_triangle(int rows) {
    int triangle[10][10];
    int i = 0;
    int j = 0;
    
    // 初始化第一列和对角线
    while (i < rows) {
        triangle[i][0] = 1;
        triangle[i][i] = 1;
        i = i + 1;
    }
    
    // 计算中间值
    i = 2;
    while (i < rows) {
        j = 1;
        while (j < i) {
            triangle[i][j] = triangle[i-1][j-1] + triangle[i-1][j];
            j = j + 1;
        }
        i = i + 1;
    }
    
    // 输出结果
    i = 0;
    while (i < rows) {
        j = 0;
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
    // 测试输入
    int n = getint();
    
    // 测试逻辑运算短路
    if (n > 0 && n <= 10) {
        pascal_triangle(n);
    } else {
        putint(-1); // 无效输入
    }
    
    // 测试shadow
    {
        int n = 3;
        pascal_triangle(n); // 输出3行
    }
    
    return 0;
}