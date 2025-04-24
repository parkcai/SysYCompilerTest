/*
 * 测试多维数组、递归函数、逻辑运算短路和变量shadow
 * 功能：计算并输出杨辉三角的前n行
 */

// 计算杨辉三角的值并填充到二维数组
void fill_pascal(int triangle[][10], int n) {
    int i = 0;
    while (i < n) {
        int j = 0;
        while (j <= i) {
            if (j == 0 || j == i) {
                triangle[i][j] = 1;  // 边界条件
            } else {
                // 利用短路特性防止数组越界
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
void print_pascal(int triangle[][10], int n) {
    int i = 0;
    while (i < n) {
        // 测试变量shadow
        {
            int i = 999;
            putint(i);  // 应输出999
            putch(10);  // 换行
        }
        
        int j = 0;
        while (j <= i) {
            putint(triangle[i][j]);
            putch(32);  // 空格
            j = j + 1;
        }
        putch(10);  // 换行
        i = i + 1;
    }
}

int main() {
    int n = getint();
    int triangle[10][10];  // 二维数组存储杨辉三角
    
    // 测试输入合法性
    if (n <= 0 || n > 10) {
        putint(-1);  // 非法输入
        return 0;
    }
    
    fill_pascal(triangle, n);
    print_pascal(triangle, n);
    
    return 0;
}