/*
 * 测试数组操作、函数递归和逻辑运算
 * 功能：实现快速排序算法并验证结果
 */

// 交换数组元素
void swap(int arr[], int i, int j) {
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

// 快速排序分区函数
int partition(int arr[], int low, int high) {
    int pivot = arr[high];
    int i = low - 1;
    int j = low;
    
    while (j < high) {
        if (arr[j] < pivot) {
            i = i + 1;
            swap(arr, i, j);
        }
        j = j + 1;
    }
    
    swap(arr, i + 1, high);
    return i + 1;
}

// 快速排序主函数
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
        if (i != size - 1) {
            putch(32); // 空格
        }
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    // 测试数组排序
    int arr[10] = {5, 8, 1, 3, 9, 6, 2, 7, 4, 0};
    const int size = 10;
    
    // 测试短路求值
    if (size > 0 && arr[0] == 5) {
        putch(84); // 'T'
    } else {
        putch(70); // 'F'
    }
    putch(10);
    
    // 排序前
    print_array(arr, size);
    
    // 排序操作
    quick_sort(arr, 0, size - 1);
    
    // 排序后
    print_array(arr, size);
    
    // 测试shadow
    {
        int size = 5;
        int arr[5] = {9, 7, 5, 3, 1};
        quick_sort(arr, 0, size - 1);
        print_array(arr, size);
    }
    
    return 0;
}