/*
 * 测试递归、数组操作、逻辑运算短路和变量shadow
 * 功能：实现并测试快速排序算法
 */

// 交换数组中的两个元素
void swap(int arr[], int i, int j) {
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

// 快速排序的分区函数
int partition(int arr[], int low, int high) {
    int pivot = arr[high];
    int i = low - 1;
    int j = low;
    
    while (j < high) {
        // 测试短路求值
        if (arr[j] <= pivot && i < j) {
            i = i + 1;
            swap(arr, i, j);
        }
        j = j + 1;
    }
    swap(arr, i + 1, high);
    return i + 1;
}

// 递归实现快速排序
void quick_sort(int arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quick_sort(arr, low, pi - 1);
        quick_sort(arr, pi + 1, high);
    }
}

// 打印数组
void print_array(int arr[], int size) {
    int i = 0;
    while (i < size) {
        putint(arr[i]);
        putch(32); // 空格
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    // 测试数组初始化和shadow
    int arr[10] = {9, 7, 5, 11, 12, 2, 14, 3, 10, 6};
    {
        int arr[5] = {1, 2, 3, 4, 5};
        print_array(arr, 5); // 输出1 2 3 4 5
    }
    
    // 测试快速排序
    quick_sort(arr, 0, 9);
    print_array(arr, 10); // 输出排序后的数组
    
    return 0;
}