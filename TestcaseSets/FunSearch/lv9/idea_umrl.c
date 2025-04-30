/*
 * 测试多维数组初始化、函数递归、逻辑运算和变量shadow
 * 功能：计算并输出螺旋矩阵
 */

// 初始化螺旋矩阵
void init_spiral(int matrix[][10], int n) {
    int value = 1;
    int top = 0, bottom = n - 1;
    int left = 0, right = n - 1;
    
    while (top <= bottom && left <= right) {
        // 从左到右填充上边
        int i = left;
        while (i <= right) {
            matrix[top][i] = value;
            value = value + 1;
            i = i + 1;
        }
        top = top + 1;
        
        // 从上到下填充右边
        i = top;
        while (i <= bottom) {
            matrix[i][right] = value;
            value = value + 1;
            i = i + 1;
        }
        right = right - 1;
        
        // 从右到左填充下边
        if (top <= bottom) {
            i = right;
            while (i >= left) {
                matrix[bottom][i] = value;
                value = value + 1;
                i = i - 1;
            }
            bottom = bottom - 1;
        }
        
        // 从下到上填充左边
        if (left <= right) {
            i = bottom;
            while (i >= top) {
                matrix[i][left] = value;
                value = value + 1;
                i = i - 1;
            }
            left = left + 1;
        }
    }
}

// 打印矩阵
void print_matrix(int matrix[][10], int n) {
    int i = 0;
    while (i < n) {
        int j = 0;
        while (j < n) {
            putint(matrix[i][j]);
            putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    int n = getint(); // 获取矩阵大小
    
    // 测试shadow变量
    {
        int n = 3;
        putint(n); // 应输出3而非输入值
        putch(10);
    }
    
    // 测试短路求值
    if (n <= 0 || n > 10) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        putch(10);
        return 0;
    }
    
    int matrix[10][10];
    init_spiral(matrix, n);
    print_matrix(matrix, n);
    
    return 0;
}