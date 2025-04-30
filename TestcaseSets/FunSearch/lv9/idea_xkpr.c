/*
 * 测试数组操作、递归、逻辑运算和变量shadow
 * 功能：实现快速排序并输出排序结果
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
    // 获取输入数组
    int arr[10];
    int n = getarray(arr);
    
    // 测试shadow变量
    {
        int n = 5;
        int arr[5] = {5,4,3,2,1};
        quick_sort(arr, 0, n - 1);
        putarray(n, arr); // 输出排序后的shadow数组
    }
    
    // 排序并输出原始输入数组
    quick_sort(arr, 0, n - 1);
    putarray(n, arr);
    
    return 0;
}