/*
 * 测试数组操作、函数递归和逻辑运算
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
        if (arr[j] <= pivot) {
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
    // 测试shadow变量
    int size = getint();
    {
        int size = 5; // shadow外层size
        int test_arr[5] = {5, 4, 3, 2, 1};
        quick_sort(test_arr, 0, size - 1);
        print_array(test_arr, size);
    }
    
    // 处理输入数组
    int arr[100];
    int actual_size = getarray(arr);
    
    // 测试短路特性
    if (actual_size > 0 && actual_size < 100) {
        quick_sort(arr, 0, actual_size - 1);
        print_array(arr, actual_size);
    } else {
        putch(33); // 输出'!'表示无效输入
    }
    
    return 0;
}