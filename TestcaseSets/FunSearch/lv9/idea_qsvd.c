/*
 * 测试数组操作、函数递归、逻辑运算短路和变量shadow
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

// 打印数组
void print_array(int arr[], int size) {
    int i = 0;
    while (i < size) {
        putint(arr[i]);
        putch(32); // 空格
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    // 测试数组
    int arr[10];
    int n = 10;
    
    // 读取输入数组
    int i = 0;
    while (i < n) {
        arr[i] = getint();
        i = i + 1;
    }
    
    // 测试shadow变量
    {
        int arr[5] = {5, 4, 3, 2, 1};
        int n = 5;
        quick_sort(arr, 0, n - 1);
        print_array(arr, n); // 应输出1 2 3 4 5
    }
    
    // 排序并打印结果
    quick_sort(arr, 0, n - 1);
    print_array(arr, n);
    
    // 测试逻辑运算短路
    if (getint() != 0 || getint() != 0) {
        putch(84); // 'T'
    } else {
        putch(70); // 'F'
    }
    
    return 0;
}