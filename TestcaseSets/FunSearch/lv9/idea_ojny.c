/*
 * 测试多维数组、递归函数、逻辑短路和变量shadow
 * 功能：计算并输出杨辉三角的前n行
 */

// 计算杨辉三角的值并填充到二维数组
void fill_pascal(int triangle[][10], int rows) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j <= i) {
            // 边界条件处理
            if (j == 0 || j == i) {
                triangle[i][j] = 1;
            } else {
                // 测试逻辑短路
                if (i > 0 && j > 0) {
                    triangle[i][j] = triangle[i-1][j-1] + triangle[i-1][j];
                }
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 递归打印数组
void print_row(int arr[], int size, int index) {
    if (index >= size) {
        return;
    }
    putint(arr[index]);
    putch(32); // 空格
    print_row(arr, size, index + 1);
}

int main() {
    // 测试shadow
    int n = getint();
    {
        int n = 5; // shadow外层n
        putint(n); // 输出5
        putch(10);
    }

    // 验证输入范围
    if (n <= 0 || n > 10) {
        putint(-1);
        return 0;
    }

    // 定义二维数组
    int triangle[10][10];
    fill_pascal(triangle, n);

    // 输出杨辉三角
    int i = 0;
    while (i < n) {
        print_row(triangle[i], i+1, 0);
        putch(10); // 换行
        i = i + 1;
    }

    return 0;
}