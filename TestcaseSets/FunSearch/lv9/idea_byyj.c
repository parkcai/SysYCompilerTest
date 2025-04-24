/*
 * 测试数组操作、函数递归和逻辑运算
 * 功能：实现并测试快速排序算法
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
    // 测试数组输入输出
    int arr[10];
    int n = getarray(arr);
    
    // 测试短路特性
    if (n > 0 && n <= 10) {
        quick_sort(arr, 0, n - 1);
        
        // 测试shadow变量
        {
            int n = 5;
            putint(n); // 应输出5
            putch(10);
        }
        
        putarray(n, arr);
    } else {
        putch(33); // 输出'!'表示无效输入
    }
    
    return 0;
}