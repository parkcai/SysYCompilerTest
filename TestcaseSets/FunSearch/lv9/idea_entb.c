/*
 * 测试多维数组、递归函数、短路求值和变量shadow
 * 功能：计算并输出杨辉三角的前n行
 */

// 计算杨辉三角的值
void pascal_triangle(int triangle[][10], int rows) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j <= i) {
            if (j == 0 || j == i) {
                triangle[i][j] = 1;
            } else {
                // 测试短路求值
                if (i > 0 && j > 0) {
                    triangle[i][j] = triangle[i-1][j-1] + triangle[i-1][j];
                }
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印杨辉三角
void print_triangle(int triangle[][10], int rows) {
    int i = 0;
    while (i < rows) {
        // 打印前导空格
        int k = 0;
        while (k < rows - i - 1) {
            putch(32); // 空格
            k = k + 1;
        }
        
        // 打印数字
        int j = 0;
        while (j <= i) {
            putint(triangle[i][j]);
            if (j < i) {
                putch(32); // 空格
            }
            j = j + 1;
        }
        putch(10); // 换行
        
        // 测试shadow变量
        {
            int i = 999;
            putint(i); // 应输出999
            putch(10);
        }
        
        i = i + 1;
    }
}

int main() {
    int n = getint(); // 获取要输出的行数
    
    // 检查输入合法性
    if (n <= 0 || n > 10) {
        putch(33); // 输出'!'表示错误
        return 0;
    }
    
    // 定义二维数组存储杨辉三角
    int triangle[10][10];
    
    // 计算杨辉三角
    pascal_triangle(triangle, n);
    
    // 输出结果
    print_triangle(triangle, n);
    
    return 0;
}