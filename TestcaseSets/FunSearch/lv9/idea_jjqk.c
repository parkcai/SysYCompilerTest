/*
 * 测试数组操作、函数递归、逻辑运算短路求值
 * 功能：实现快速排序并输出排序结果
 */

// 交换两个数组元素的值
void swap(int arr[], int i, int j) {
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

// 快速排序的partition函数
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
void quickSort(int arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}

int main() {
    int arr[10];
    int n = getarray(arr); // 获取输入数组
    
    // 测试短路求值
    if (n <= 0 || n > 10) {
        putint(-1); // 输入不合法
        return 0;
    }
    
    // 排序前输出
    putarray(n, arr);
    
    // 执行快速排序
    quickSort(arr, 0, n - 1);
    
    // 排序后输出
    putarray(n, arr);
    
    // 测试shadow
    {
        int n = 123;
        putint(n); // 应输出123
    }
    
    return 0;
}