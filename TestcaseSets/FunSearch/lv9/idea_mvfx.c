/*
 * 测试数组操作、逻辑运算短路特性、函数递归和变量shadow
 * 功能：实现选择排序并验证结果，同时测试逻辑运算短路特性和多维数组
 */

// 交换数组元素
void swap(int arr[], int i, int j) {
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

// 选择排序实现
void selection_sort(int arr[], int len) {
    int i = 0;
    while (i < len - 1) {
        int min_idx = i;
        int j = i + 1;
        while (j < len) {
            if (arr[j] < arr[min_idx]) {
                min_idx = j;
            }
            j = j + 1;
        }
        if (min_idx != i) {
            swap(arr, i, min_idx);  // 调用交换函数
        }
        i = i + 1;
    }
}

// 打印数组
void print_array(int arr[], int len) {
    int i = 0;
    while (i < len) {
        putint(arr[i]);
        if (i != len - 1) {
            putch(32);  // 输出空格分隔
        }
        i = i + 1;
    }
    putch(10);  // 换行
}

// 初始化二维数组
void init_2d_array(int mat[][3], int rows, int val) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < 3) {
            mat[i][j] = val + i * 3 + j;
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印二维数组
void print_2d_array(int mat[][3], int rows) {
    int i = 0;
    while (i < rows) {
        int j = 0;
        while (j < 3) {
            putint(mat[i][j]);
            putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

int main() {
    int arr[5] = {5, 3, 4, 1, 2};
    int mat[3][3];
    
    // 测试选择排序
    putch(83);  // 'S'
    putch(111); // 'o'
    putch(114); // 't'
    putch(116); // 't'
    putch(105); // 'i'
    putch(110); // 'n'
    putch(103); // 'g'
    putch(58);  // ':'
    putch(10);  // 换行
    
    print_array(arr, 5);  // 排序前输出
    selection_sort(arr, 5);  // 调用排序函数
    print_array(arr, 5);  // 排序后输出
    
    // 测试初始化和打印二维数组
    init_2d_array(mat, 3, 1);
    print_2d_array(mat, 3);
    
    // 测试逻辑运算短路特性
    int a = getint();
    int b = getint();
    if (a > 0 && b > 0 || a == b) {
        putint(1);
    } else {
        putint(0);
    }
    
    // 测试变量shadow
    {
        int a = 5;
        putint(a); // 应输出5
        putch(10);
    }
    
    return 0;
}