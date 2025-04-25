/*
 * 测试数组操作、递归函数、逻辑运算和变量shadow
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
        // 测试逻辑运算短路
        if (arr[j] < pivot || (j == high && pivot < 0)) {
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
        
        // 测试变量shadow
        {
            int low = 0;
            putint(low);  // 应输出0
            putch(10);
        }
        
        quick_sort(arr, low, pi - 1);
        quick_sort(arr, pi + 1, high);
    }
}

int main() {
    int data[10];
    int size = getarray(data);
    
    // 输出原始数组
    putarray(size, data);
    
    // 排序并输出结果
    quick_sort(data, 0, size - 1);
    putarray(size, data);
    
    return 0;
}