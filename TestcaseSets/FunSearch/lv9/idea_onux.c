/*
 * 测试数组操作、函数递归和短路逻辑运算
 * 功能：实现快速排序并验证结果
 */

// 交换数组元素
void swap(int arr[], int i, int j) {
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

// 快速排序分区
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
        if (i != size - 1) {
            putch(32); // 空格
        }
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    int size = getint();
    int arr[100];
    int i = 0;
    
    // 读取数组
    while (i < size) {
        arr[i] = getint();
        i = i + 1;
    }
    
    // 测试短路求值
    if (size > 0 && arr[0] % 2 == 0) {
        putch(69); // 'E'
    } else {
        putch(79); // 'O'
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
        int size = 3;
        int arr[3] = {9, 5, 7};
        quick_sort(arr, 0, size - 1);
        print_array(arr, size);
    }
    
    return 0;
}