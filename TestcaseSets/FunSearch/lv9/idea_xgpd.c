/*
 * 测试多维数组、函数递归、逻辑运算和库函数综合应用
 * 功能：计算并输出杨辉三角的前n行
 */

// 计算杨辉三角的值并存入二维数组
void pascal_triangle(int arr[][10], int n) {
    int i = 0;
    while (i < n) {
        int j = 0;
        while (j <= i) {
            if (j == 0 || j == i) {
                arr[i][j] = 1; // 边界条件
            } else {
                // 递归关系
                arr[i][j] = arr[i-1][j-1] + arr[i-1][j];
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 输出二维数组
void print_array(int arr[][10], int n) {
    int i = 0;
    while (i < n) {
        int j = 0;
        while (j <= i) {
            putint(arr[i][j]);
            putch(32); // 输出空格
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    int n = getint(); // 获取杨辉三角的行数
    int triangle[10][10]; // 定义二维数组
    
    // 测试输入合法性
    if (n <= 0 || n > 10) {
        putch(33); // 输出'!'表示错误
        return -1;
    }
    
    // 计算杨辉三角
    pascal_triangle(triangle, n);
    
    // 输出结果
    print_array(triangle, n);
    
    // 测试shadow
    {
        int n = 5;
        putint(n); // 应输出5而非输入值
        putch(10);
    }
    
    return 0;
}