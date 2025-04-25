/*
 * 测试数组操作、函数递归、短路逻辑和变量shadow
 * 功能：实现快速排序并输出排序结果
 */

// 交换数组元素
void swap(int arr[], int i, int j) {
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

// 分区函数
int partition(int arr[], int low, int high) {
    int pivot = arr[high];
    int i = low - 1;
    int j = low;
    
    while (j < high) {
        // 测试短路逻辑：先检查j < high防止越界
        if (j < high && arr[j] < pivot) {
            i = i + 1;
            swap(arr, i, j);
        }
        j = j + 1;
    }
    swap(arr, i + 1, high);
    return i + 1;
}

// 快速排序递归实现
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
    int arr[10] = {7, 2, 9, 1, 5, 3, 8, 4, 6, 0};
    int size = 10;
    
    // 排序前输出
    print_array(arr, size);
    
    // 执行排序
    quick_sort(arr, 0, size - 1);
    
    // 测试shadow变量
    {
        int arr[5] = {5, 4, 3, 2, 1};
        int size = 5;
        quick_sort(arr, 0, size - 1);
        print_array(arr, size); // 应输出1 2 3 4 5
    }
    
    // 排序后输出
    print_array(arr, size);
    
    return 0;
}