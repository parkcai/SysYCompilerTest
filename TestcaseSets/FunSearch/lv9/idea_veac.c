/*
 * 测试一维数组操作、逻辑运算和函数调用
 * 功能：实现简单选择排序算法并验证
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

int main() {
    int arr[5] = {5, 3, 4, 1, 2};
    
    // 排序前输出
    putch(83);  // 'S'
    putch(111); // 'o'
    putch(114); // 'r'
    putch(116); // 't'
    putch(105); // 'i'
    putch(110); // 'n'
    putch(103); // 'g'
    putch(58);  // ':'
    putch(10);  // 换行
    print_array(arr, 5);
    
    // 调用排序函数
    selection_sort(arr, 5);
    
    // 排序后输出
    print_array(arr, 5);
    
    // 测试逻辑运算
    int a = getint();
    int b = getint();
    if (a > 0 && b > 0 || a == b) {
        putint(1);
    } else {
        putint(0);
    }
    
    return 0;
}