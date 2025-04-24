/*
 * 测试数组操作、函数递归和逻辑运算
 * 功能：实现快速排序并验证排序结果
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

// 快速排序递归实现
void quick_sort(int arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quick_sort(arr, low, pi - 1);
        quick_sort(arr, pi + 1, high);
    }
}

int main() {
    int data[10];
    int size = getarray(data); // 获取输入数组
    
    // 排序前输出
    putarray(size, data);
    putch(10); // 换行
    
    // 快速排序
    quick_sort(data, 0, size - 1);
    
    // 排序后输出
    putarray(size, data);
    
    // 测试shadow和短路求值
    {
        int size = 3;
        if (size > 0 && (100 / size > 0)) {
            putch(10);
            putint(size); // 应输出3
        }
    }
    
    return 0;
}