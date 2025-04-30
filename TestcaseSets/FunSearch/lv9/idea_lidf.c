/*
 * 测试多维数组操作、逻辑运算、变量shadow和库函数的使用
 * 功能：实现一个简单的二维数组排序，并验证结果
 */

// 交换数组元素
void swap(int arr[][3], int i, int j, int k) {
    int temp = arr[i][j];
    arr[i][j] = arr[k][j];
    arr[k][j] = temp;
}

// 对二维数组的一列进行冒泡排序
void bubble_sort_column(int arr[][3], int col, int rows) {
    int i = 0;
    while (i < rows - 1) {
        int j = 0;
        while (j < rows - 1 - i) {
            if (arr[j][col] > arr[j + 1][col]) {
                swap(arr, j, col, j + 1);
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印二维数组
void print_2d_array(int arr[][3], int rows) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < 3) {
            putint(arr[i][j]);
            if (j < 2) {
                putch(32); // 空格
            }
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    // 定义一个3x3的二维数组
    int matrix[3][3] = {{5, 3, 8}, {1, 2, 4}, {7, 6, 9}};

    // 测试shadow变量
    {
        int matrix = 42; // shadow外层的matrix
        putint(matrix);  // 应输出42
        putch(10);       // 换行
    }

    // 排序前打印
    putch(79); // 'O'
    putch(114); // 'r'
    putch(105); // 'i'
    putch(103); // 'g'
    putch(105); // 'i'
    putch(110); // 'n'
    putch(97); // 'a'
    putch(108); // 'l'
    putch(58); // ':'
    putch(10); // 换行
    print_2d_array(matrix, 3);

    // 对每一列进行排序
    int col = 0;
    while (col < 3) {
        bubble_sort_column(matrix, col, 3);
        col = col + 1;
    }

    // 排序后打印
    putch(83); // 'S'
    putch(111); // 'o'
    putch(114); // 'r'
    putch(116); // 't'
    putch(101); // 'e'
    putch(100); // 'd'
    putch(58); // ':'
    putch(10); // 换行
    print_2d_array(matrix, 3);

    // 测试短路逻辑
    if (matrix[0][0] == 1 && matrix[2][2] == 9 || matrix[1][1] != 2) {
        putch(33); // '!'
        putch(10); // 换行
    }

    return 0;
}