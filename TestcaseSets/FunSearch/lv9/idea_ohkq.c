/*
 * 测试多维数组、函数递归和复杂逻辑表达式
 * 功能：计算并输出杨辉三角的前n行
 */

// 计算杨辉三角的值
void pascal_triangle(int rows, int arr[][10]) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j <= i) {
            if (j == 0 || j == i) {
                arr[i][j] = 1;
            } else {
                arr[i][j] = arr[i-1][j-1] + arr[i-1][j];
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印数组
void print_array(int rows, int arr[][10]) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j <= i) {
            putint(arr[i][j]);
            putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    int n = getint(); // 获取行数
    
    // 测试逻辑表达式短路
    if (n <= 0 || n > 10) {
        putch(69); // 'E'
        putch(114); // 'r'
        putch(114); // 'r'
        putch(111); // 'o'
        putch(114); // 'r'
        return 0;
    }
    
    // 定义二维数组
    int triangle[10][10];
    
    // 测试shadow变量
    {
        int n = 3;
        putint(n); // 应输出3
        putch(10); // 换行
    }
    
    pascal_triangle(n, triangle);
    print_array(n, triangle);
    
    return 0;
}