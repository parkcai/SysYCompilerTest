/*
 * 测试数组操作、函数调用和逻辑运算
 * 功能：实现并测试冒泡排序算法
 */

// 交换两个元素的值
void swap(int a[], int i, int j) {
    int temp = a[i];
    a[i] = a[j];
    a[j] = temp;
}

// 冒泡排序
void bubble_sort(int arr[], int len) {
    int i = 0;
    while (i < len - 1) {
        int j = 0;
        while (j < len - i - 1) {
            if (arr[j] > arr[j + 1]) {
                swap(arr, j, j + 1);
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印数组
void print_array(int arr[], int len) {
    int i = 0;
    while (i < len) {
        putint(arr[i]);
        if (i < len - 1) {
            putch(32); // 空格
        }
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    // 测试数组
    int arr[10] = {5, 2, 9, 1, 5, 6, 3, 8, 4, 7};
    
    // 排序前
    print_array(arr, 10);
    
    // 排序
    bubble_sort(arr, 10);
    
    // 排序后
    print_array(arr, 10);
    
    // 测试shadow变量
    {
        int arr[5] = {5, 4, 3, 2, 1};
        bubble_sort(arr, 5);
        print_array(arr, 5);
    }
    
    return 0;
}