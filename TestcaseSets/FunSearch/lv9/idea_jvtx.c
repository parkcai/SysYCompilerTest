/*
 * 测试多维数组、递归函数、逻辑运算短路和变量shadow
 * 功能：计算并输出帕斯卡三角形的前n行
 */

// 递归计算帕斯卡三角形的值
void compute_pascal(int triangle[][10], int row, int col) {
    if (row == 0 || col == 0 || row == col) {
        triangle[row][col] = 1;
        return;
    }
    
    // 递归计算上一行的两个值
    compute_pascal(triangle, row - 1, col - 1);
    compute_pascal(triangle, row - 1, col);
    
    // 当前值为上一行两个值之和
    triangle[row][col] = triangle[row - 1][col - 1] + triangle[row - 1][col];
}

// 打印帕斯卡三角形
void print_pascal(int triangle[][10], int n) {
    int i = 0;
    while (i < n) {
        // 打印前导空格
        int j = 0;
        while (j < n - i - 1) {
            putch(32); // 空格
            j = j + 1;
        }
        
        // 打印数字
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
    int n = 5; // 输出5行帕斯卡三角形
    int triangle[10][10]; // 假设最多10行
    
    // 初始化三角形
    int i = 0;
    while (i < n) {
        int j = 0;
        while (j <= i) {
            compute_pascal(triangle, i, j);
            j = j + 1;
        }
        i = i + 1;
    }
    
    // 测试shadow变量
    {
        int i = 0;
        while (i < n) {
            // 每行第一个元素加1
            triangle[i][0] = triangle[i][0] + 1;
            i = i + 1;
        }
    }
    
    // 打印三角形
    print_pascal(triangle, n);
    
    return 0;
}