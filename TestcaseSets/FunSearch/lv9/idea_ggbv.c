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

// 快速排序主函数
void quick_sort(int arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quick_sort(arr, low, pi - 1);
        quick_sort(arr, pi + 1, high);
    }
}

int main() {
    int arr[10];
    int size = getarray(arr);
    
    // 测试短路求值
    if (size > 0 && size <= 10) {
        quick_sort(arr, 0, size - 1);
        putarray(size, arr);
    } else {
        putch(33); // 输出'!'表示无效输入
    }
    
    // 测试shadow变量
    {
        int size = 3;
        int arr[3] = {9, 6, 3};
        quick_sort(arr, 0, size - 1);
        putarray(size, arr); // 应输出3 6 9
    }
    
    return 0;
}