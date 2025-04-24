/*
 * 测试多维数组、函数递归和逻辑运算短路
 * 功能：计算并输出帕斯卡三角形
 */

// 计算帕斯卡三角形
void pascal_triangle(int n, int triangle[][10]) {
    int i = 0;
    while (i < n) {
        int j = 0;
        while (j <= i) {
            if (j == 0 || j == i) {
                triangle[i][j] = 1;
            } else {
                // 测试逻辑运算短路：当i>0时才会访问triangle[i-1]
                triangle[i][j] = (i > 0) && triangle[i-1][j-1] + triangle[i-1][j];
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 递归打印数组
void print_array_recursive(int arr[], int size, int index) {
    if (index >= size) {
        putch(10); // 换行
        return;
    }
    putint(arr[index]);
    putch(32); // 空格
    print_array_recursive(arr, size, index + 1); // 测试递归调用
}

int main() {
    const int ROWS = 5;
    int triangle[10][10]; // 二维数组测试
    
    // 计算帕斯卡三角形
    pascal_triangle(ROWS, triangle);
    
    // 输出结果
    int i = 0;
    while (i < ROWS) {
        print_array_recursive(triangle[i], i+1, 0);
        i = i + 1;
    }
    
    // 测试shadow
    {
        int ROWS = 3;
        putch(10);
        print_array_recursive(triangle[ROWS], ROWS+1, 0); // 输出第3行
    }
    
    return 0;
}