/*
 * 测试数组操作、函数递归和逻辑运算短路
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
        // 利用短路特性防止数组越界
        if (j >= 0 && arr[j] < pivot) {
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
    // 测试数组
    int arr[10];
    int n = 10;
    
    // 初始化数组
    int i = 0;
    while (i < n) {
        arr[i] = getint();
        i = i + 1;
    }
    
    // 排序前输出
    putarray(n, arr);
    
    // 执行快速排序
    quick_sort(arr, 0, n - 1);
    
    // 排序后输出
    putarray(n, arr);
    
    // 测试shadow变量
    {
        int arr[5] = {5, 4, 3, 2, 1};
        quick_sort(arr, 0, 4);
        putarray(5, arr); // 应输出1 2 3 4 5
    }
    
    return 0;
}